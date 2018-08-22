// Copyright © 2017-2018 Dmitriy Khaustov
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

// HttpServer.cpp


#include <sstream>
#include <memory>
#include <cstring>
#include "../../net/ConnectionManager.hpp"
#include "../../net/TcpConnection.hpp"
#include "../../server/Server.hpp"
#include "HttpContext.hpp"
#include "HttpServer.hpp"

REGISTER_TRANSPORT(http, HttpServer);

bool HttpServer::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for HttpServer");
	}

	int n = 0;

	// Цикл обработки запросов
	for (;;)
	{
		if (!connection->getContext())
		{
			connection->setContext(std::make_shared<HttpContext>(connection));
		}
		auto context = std::dynamic_pointer_cast<HttpContext>(connection->getContext());
		if (!context)
		{
			throw std::runtime_error("Bad context-type for this transport");
		}

		// Незавершенного запроса нет
		if (!context->getRequest())
		{
			if (connection->dataLen() >= 4)
			{
				if (
					strncasecmp(connection->dataPtr(), "GET", 3) &&
					strncasecmp(connection->dataPtr(), "POST", 4)
				)
				{
					_log.debug("Bad request");

					HttpResponse(400)
						<< HttpHeader("Connection", "Close")
						<< "Bad request ← Bad request line ← Bad method ← Unknown method" << "\r\n"
						>> *connection;

					_log.info("HTTP Unsupported method in request line");

					connection->setTtl(std::chrono::milliseconds(50));
					break;
				}
			}

			// Проверка готовности заголовков
			auto endHeaders = static_cast<const char*>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

			if (!endHeaders && connection->dataLen() < (1ull << 12))
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

			if (!endHeaders || (endHeaders - connection->dataPtr()) > (1ll << 12))
			{
				_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

				HttpResponse(400)
					<< HttpHeader("Connection", "Close")
					<< "Headers data too large\r\n"
					>> *connection;

				_log.info("HTTP Bad request: headers too large");

				connection->setTtl(std::chrono::milliseconds(50));
				break;
			}

			size_t headersSize = endHeaders - connection->dataPtr() + 4;

			_log.debug("Read %zu bytes of request headers", headersSize);

			try
			{
				// Читаем запрос
				auto request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);

				if (!context->isHttp100ContinueSent())
				{
					if (strcasestr(request->getHeader("Expect").c_str(), "100-Continue") != nullptr)
					{
						HttpResponse(100, "Continue", request->protocol())
							>> *connection;

						_log.info("HTTP 100-Continue");

						context->setHttp100ContinueSent();
						connection->setTtl(std::chrono::seconds(10));
					}
				}

				context->setRequest(request);
			}
			catch (std::exception& exception)
			{
				HttpResponse(400)
					<< HttpHeader("Connection", "Close")
					<< exception.what() << "\r\n"
					>> *connection;

				connection->setTtl(std::chrono::milliseconds(50));

				_log.debug("Bad request");
				_log.info("HTTP Bad request");
				break;
			}

			// Пропускаем байты заголовка
			connection->skip(headersSize);

			connection->setTtl(std::chrono::seconds(5));
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
						connection->setTtl(std::chrono::seconds(5));

						_log.debug("Not anough data for read request body (%zu < %zu)",
									context->getRequest()->dataLen(), context->getRequest()->contentLength());
						return true;
					}
				}
			}
			else if (context->getRequest()->ifTransferEncoding(HttpRequest::TransferEncoding::CHUNKED))
			{
				for (;;)
				{
					// Заголовок чанка (размер)
					auto endHeader = static_cast<const char*>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n", 2));
					if (endHeader == nullptr)
					{
						connection->setTtl(std::chrono::seconds(5));
						_log.debug("Not anough data for read head of chunk");
						return true;
					}

					// Размер чанка
					uint64_t size = 0;
					std::string header(connection->dataPtr(), endHeader);
					try
					{
						size = std::stoull(header, nullptr, 16);
					}
					catch (...)
					{
						HttpResponse(400)
							<< HttpHeader("Connection", "Close")
							<< "Wrong head of chunk for chunked Transfer-Encoding" << "\r\n"
							>> *connection;

						_log.debug("Wrong head of chunk for chunked Transfer-Encoding");
						connection->setTtl(std::chrono::milliseconds(50));
						return true;
					}

					// Слишком большой чанк
					if (size > (1ull << 22)) // 4Mb
					{
						HttpResponse(500)
							<< HttpHeader("Connection", "Close")
							<< "Too large chunk for chunked Transfer-Encoding" << "\r\n"
							>> *connection;

						_log.debug("Too large chunk for chunked Transfer-Encoding");
						connection->setTtl(std::chrono::milliseconds(50));
						return true;
					}

					// Недостаточно данных для получения чанка целиком
					if (connection->dataLen() < (endHeader - connection->dataPtr()) + 2 + size + 2)
					{
						connection->setTtl(std::chrono::seconds(5));
						_log.debug("Not anough data for read body of chunk");
						return true;
					}

					// Некорректный терминатор чанка
					if (strcmp(endHeader + 2 + size, "\r\n"))
					{
						HttpResponse(400)
							<< HttpHeader("Connection", "Close")
							<< "Wrong end of chunk for chunked Transfer-Encoding" << "\r\n"
							>> *connection;

						_log.debug("Wrong end of chunk for chunked Transfer-Encoding");
						connection->setTtl(std::chrono::milliseconds(50));
						return true;
					}

					context->getRequest()->write(endHeader + 2, size);

					connection->skip(endHeader + 2 + size + 2 - connection->dataPtr());

					// Чанк нулевого размера
					if (size == 0)
					{
						break;
					}

					// Данных больше не будет
					if (connection->noRead())
					{
						HttpResponse(400)
							<< HttpHeader("Connection", "Close")
							<< "Incomplete chunked body of request" << "\r\n"
							>> *connection;

						_log.debug("Incomplete chunked body of request");
						connection->setTtl(std::chrono::milliseconds(50));
						return true;
					}
				}
			}
			else
			{
				context->getRequest()->write(connection->dataPtr(), connection->dataLen());

				connection->skip(connection->dataLen());

				if (!connection->noRead())
				{
					connection->setTtl(std::chrono::seconds(5));

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
			auto handler = getHandler(context->getRequest()->uri().path());

			if (!handler)
			{
				HttpResponse(404, "Not Found", context->getRequest()->protocol())
					<< HttpHeader("Connection", "Close")
					<< "Not found service-handler for uri " << context->getRequest()->uri().path() << "\r\n"
					>> *connection;

				_log.debug("RESPONSE: 404 Not found service-handler for uri %s", context->getRequest()->uri().path().c_str());
				_log.info("HTTP Not found '%s' for '%s'", context->getRequest()->uri().path().c_str(), context->getRequest()->uri().str().c_str());

				connection->resetContext();
				connection->setTtl(std::chrono::milliseconds(50));
				return true;
			}

			context->setHandler(std::move(handler));

			context->setTransmitter(
				std::make_shared<Transport::Transmitter>(
					[this,connection]
					(const char*data, size_t size, const std::string& contentType, bool close)
					{

						std::string out(data, size);
						HttpResponse response(200);
						if (close)
						{
							response << HttpHeader("Connection", "Close");
							connection->setTtl(std::chrono::milliseconds(50));
						}
						else
						{
							response << HttpHeader("Keep-Alive", "timeout=15, max=100");
							connection->setTtl(std::chrono::seconds(20));
						}
						if (!contentType.empty())
						{
							response << HttpHeader("Content-Type", contentType);
						}
						response
							<< out << "\r\n"
							>> *connection;

						_log.debug("RESPONSE: 200");
					}
				)
			);

			if (metricRequestCount) metricRequestCount->addValue();
			if (metricAvgRequestPerSec) metricAvgRequestPerSec->addValue();
			auto beginTime = std::chrono::steady_clock::now();

			context->handle();

			auto now = std::chrono::steady_clock::now();
			auto timeSpent =
				static_cast<double>(std::chrono::steady_clock::duration(now - beginTime).count()) /
				static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
			if (timeSpent > 0)
			{
				if (metricAvgExecutionTime) metricAvgExecutionTime->addValue(timeSpent, now);
			}

			connection->resetContext();
			n++;
			continue;
		}
		catch (const std::exception& exception)
		{
			HttpResponse(500, "", context->getRequest() ? context->getRequest()->protocol() : 100)
				<< HttpHeader("Connection", "Close")
				<< HttpHeader("Content-Type", "text/plain;charset=utf-8")
				<< "500 Internal Server Error:\n  " << exception.what() << "\r\n"
				>> *connection;

			connection->setTtl(std::chrono::milliseconds(50));

			_log.debug("RESPONSE: 200");
		}

		connection->resetContext();
		n++;
	}

	_log.debug("Processed %d request, remain %zu bytes", n, connection->dataLen());

	return false;
}

void HttpServer::bindHandler(const std::string& selector, const std::shared_ptr<ServerTransport::Handler>& handler)
{
	if (_handlers.find(selector) != _handlers.end())
	{
		throw std::runtime_error("Handler already set early");
	}

	_handlers.emplace(selector, handler);
}

std::shared_ptr<ServerTransport::Handler> HttpServer::getHandler(const std::string& subject_)
{
	auto subject = subject_;
	do
	{
		auto i = _handlers.upper_bound(subject);

		if (i != _handlers.end())
		{
			if (i->first == subject)
			{
				return i->second;
			}
		}

		if (i == _handlers.begin())
		{
			break;
		}

		--i;

		size_t len = std::min(subject.length(), i->first.length());

		int cmpres = std::strncmp(i->first.c_str(), subject.c_str(), len);
		if (cmpres == 0)
		{
			return i->second;
		}
		if (cmpres > 0)
		{
			return nullptr;
		}

		subject.resize(--len);
	}
	while (subject.length());

	return nullptr;
}
