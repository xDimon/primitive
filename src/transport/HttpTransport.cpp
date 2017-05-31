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
#include "http/HttpResponse.hpp"

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
			connection->setContext(std::make_shared<HttpContext>()->ptr());
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
			auto endHeaders = static_cast<const char*>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

			if (!endHeaders && connection->dataLen() < (1 << 12))
			{
				if (connection->dataLen())
				{
					_log.debug("Not anough data for read headers (%zu bytes)", connection->dataLen());
				}
				else
				{
					_log.debug("No more data");
				}
				break;
			}

			if (!endHeaders || (endHeaders - connection->dataPtr()) > (1 << 12))
			{
				_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

				HttpResponse(400)
					<< HttpHeader("Connection", "Close")
					<< "Headers data too large\r\n"
					>> *connection;

				break;
			}

			size_t headersSize = endHeaders - connection->dataPtr() + 4;

			_log.debug("Read %zu bytes of request headers", headersSize);

			try
			{
				// Читаем запрос
				auto request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);

				context->setRequest(request);
			}
			catch (std::runtime_error& exception)
			{
				HttpResponse(400)
					<< HttpHeader("Connection", "Close")
					<< exception.what() << "\r\n"
					>> *connection;

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
					size_t len = std::min(context->getRequest()->contentLength(), connection->dataLen());

					context->getRequest()->write(connection->dataPtr(), len);

					connection->skip(len);

					if (context->getRequest()->contentLength() > context->getRequest()->dataLen())
					{
						_log.debug(
							"Not anough data for read request body ({%zu < %zu)",
							context->getRequest()->dataLen(),
							context->getRequest()->contentLength()
						);
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
					_log.debug("Not read all request body yet (read %zu)", context->getRequest()->dataLen());
					break;
				}
			}
		}

		_log.debug("REQUEST: %s %s %zu",
			context->getRequest()->method_s().c_str(),
			context->getRequest()->uri_s().c_str(),
			context->getRequest()->hasContentLength() ? context->getRequest()->contentLength() : 0
		);

		try
		{
//			auto handler = getHandler(context->getRequest()->uri().path());

			const std::vector<char> out;

//			out = handler(context->getRequest()->data());

			// TODO реализовать реальную обработку вместо echo
			HttpResponse(200)
				<< "Received:\r\n"
//				<< out << "\r\n"
				>> *connection;
		}
		catch (const std::exception& exception)
		{
			SObj output;
			output.insert("status", new SStr("error"));
			output.insert("error", new SStr(exception.what()));

			HttpResponse(200)
				<< HttpHeader("Connection", "Close")
//				<< serializer()->encode(&output) << "\r\n"
				>> *connection;
		}

		context->getRequest().reset();

		n++;
	}

	_log.debug("Processed %d request, remain %zu bytes", n, connection->dataLen());

	return false;
}
