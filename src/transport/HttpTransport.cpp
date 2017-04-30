// Copyright © 2017 Dmitriy Khaustov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: khaustov.dm@gmail.com
// File created on: 2017.03.27

// HttpTransport.cpp


#include <sstream>
#include <memory>
#include "../net/ConnectionManager.hpp"
#include "../net/TcpConnection.hpp"
#include "../server/Server.hpp"
#include "../utils/Packet.hpp"
#include "../utils/Time.hpp"
#include "http/HttpContext.hpp"
#include "HttpTransport.hpp"
#include "../serialization/SObj.hpp"

bool HttpTransport::processing(std::shared_ptr<Connection> connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	int n = 0;

	// Цикл обработки запросов
	for (;;)
	{
		if (!connection->getContext())
		{
			auto context = HttpContext::Ptr(new HttpContext());

			connection->setContext(context);
		}
		auto context = std::dynamic_pointer_cast<HttpContext>(connection->getContext());
		if (!context)
		{
			throw std::runtime_error("Bad context-type for this transport");
		}

		// Незавершенного запроса нет
		if (!context->getRequest())
		{
			// Проверка готовности заголовков
			auto endHeaders = static_cast<const char *>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

			if (!endHeaders && connection->dataLen() < (1 << 12))
			{
				if (connection->dataLen())
				{
					log().debug("Not anough data for read headers (%zu bytes)", connection->dataLen());
				}
				else
				{
					log().debug("No more data");
				}
				break;
			}

			if (!endHeaders || (endHeaders - connection->dataPtr()) > (1 << 12))
			{
				log().debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

				std::stringstream ss;
				ss << "HTTP/1.0 400 Bad request\r\n"
					<< "Server: " << Server::httpName() << "\r\n"
					<< "Date: " << Time::httpDate() << "\r\n"
					<< "X-Transport: http\r\n"
				   << "\r\n"
				   << "Headers data too large" << "\r\n";

				// Формируем пакет
				Packet response(ss.str().c_str(), ss.str().size());

				// Отправляем
				connection->write(response.data(), response.size());

				connection->close();

				break;
			}

			size_t headersSize = endHeaders - connection->dataPtr() + 4;

			log().debug("Read %zu bytes of request headers", headersSize);

			try
			{
				// Читаем запрос
				auto request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);

				context->setRequest(request);
			}
			catch (std::runtime_error& exception)
			{
				std::stringstream ss;
				ss << "HTTP/1.0 400 Bad request\r\n"
					<< "Server: " << Server::httpName() << "\r\n"
					<< "Date: " << Time::httpDate() << "\r\n"
					<< "X-Transport: http\r\n"
				   << "\r\n"
				   << exception.what() << "\r\n";

				// Формируем пакет
				Packet response(ss.str().c_str(), ss.str().size());

				// Отправляем
				connection->write(response.data(), response.size());

				connection->close();

				break;
			}

			// Пропускаем байты заголовка
			connection->skip(headersSize);
		}

		// Читаем тело запроса
		if (context->getRequest()->method() == HttpRequest::Method::POST)
		{
			if (context->getRequest()->hasContentLength())
			{
				if (context->getRequest()->contentLength() > context->getRequest()->dataLen())
				{
					size_t len = std::min(context->getRequest()->contentLength(), context->getRequest()->dataLen());

					context->getRequest()->write(connection->dataPtr(), len);

					connection->skip(len);

					if (context->getRequest()->contentLength() > context->getRequest()->dataLen())
					{
						log().debug("Not anough data for read request body ({%zu < %zu)", context->getRequest()->dataLen(), context->getRequest()->contentLength());
						break;
					}
				}
			}
			else
			{
				context->getRequest()->write(connection->dataPtr(), connection->dataLen());

				connection->skip(connection->dataLen());

				if (!connection->noRead())
				{
					log().debug("Not read all request body yet (read %zu)", context->getRequest()->dataLen());
					break;
				}
			}
		}

		log().debug("REQUEST: %s %s %zu", context->getRequest()->method_s().c_str(), context->getRequest()->uri_s().c_str(), context->getRequest()->hasContentLength() ? context->getRequest()->contentLength() : 0);

		try
		{
			std::shared_ptr<SVal> input;

			if (context->getRequest()->dataLen())
			{
				auto data = context->getRequest()->data();
				input.reset(serializer()->decode({data.begin(), data.end()}));
			}
			else if (context->getRequest()->uri().hasQuery())
			{
				auto data = HttpUri::urldecode(context->getRequest()->uri().query());
				input.reset(serializer()->decode(data));
			}
			else
			{
				throw std::runtime_error("No data for parsing");
			}

			std::ostringstream body;
			body << "Received:\r\n"
				 << serializer()->encode(input.get()) << "\r\n";

			std::stringstream ss;
			ss << "HTTP/1.0 200 OK\r\n"
			   << "Server: " << Server::httpName() << "\r\n"
			   << "Date: " << Time::httpDate() << "\r\n"
			   << "X-Transport: http\r\n"
			   << "Content-Length: " << body.str().length() << "\r\n"
			   << "\r\n"
			   << body.str();

			// Отправляем
			connection->write(ss.str().c_str(), ss.str().length());
		}
		catch (const std::exception& exception)
		{
			SObj output;
			output.insert(new SStr("status"), new SStr("error"));
			output.insert(new SStr("error"), new SStr(exception.what()));

			auto body = serializer()->encode(&output);

			std::ostringstream ss;
			ss << "HTTP/1.0 400 OK\r\n"
			   << "Server: " << Server::httpName() << "\r\n"
			   << "Date: " << Time::httpDate() << "\r\n"
			   << "X-Transport: http\r\n"
			   << "Content-Length: " << body.length() << "\r\n"
			   << "\r\n"
			   << body;

			// Отправляем
			connection->write(ss.str().c_str(), ss.str().length());

			connection->close();
		}

		context->getRequest().reset();

		n++;
	}

	log().debug("Processed %d request, remain %zu bytes", n, connection->dataLen());

	return false;
}
