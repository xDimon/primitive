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
// File created on: 2017.06.06

// HttpClient.cpp


#include <string.h>
#include "HttpClient.hpp"
#include "../../net/TcpConnection.hpp"
#include "HttpContext.hpp"

bool HttpClient::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for HttpClient");
	}

	int n = 0;

	// Цикл обработки ответа
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

		// Незавершенного ответа нет
		if (!context->getResponse())
		{
			// Проверка готовности заголовков
			auto endHeaders = static_cast<const char*>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

			if (endHeaders == nullptr && connection->dataLen() < (1ull << 12))
			{
				if (connection->dataLen() > 0)
				{
					_log.debug("Not anough data for read headers (%zu bytes)", connection->dataLen());
				}
				else
				{
					_log.debug("No more data");
				}
				break;
			}

			if (endHeaders == nullptr  || (endHeaders - connection->dataPtr()) > (1ll << 12))
			{
				_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

				connection->setTtl(std::chrono::milliseconds(50));
				break;
			}

			size_t headersSize = endHeaders - connection->dataPtr() + 4;

			_log.debug("Read %zu bytes of request headers", headersSize);

			try
			{
				// Читаем ответ
				auto response = std::make_shared<HttpResponse>(connection->dataPtr(), connection->dataPtr() + headersSize);

				context->setResponse(response);
			}
			catch (std::exception& exception)
			{
				connection->setTtl(std::chrono::milliseconds(50));
				break;
			}

			// Пропускаем байты заголовка
			connection->skip(headersSize);
		}

		// Читаем тело ответа
		if (context->getResponse()->hasContentLength())
		{
			if (context->getResponse()->contentLength() > context->getResponse()->dataLen())
			{
				size_t len = std::min(context->getResponse()->contentLength(), connection->dataLen());

				context->getResponse()->write(connection->dataPtr(), len);

				connection->skip(len);

				if (context->getResponse()->contentLength() > context->getResponse()->dataLen())
				{
					_log.debug("Not anough data for read request body (%zu < %zu)",
								context->getResponse()->dataLen(), context->getResponse()->contentLength());
					return true;
				}
			}
		}
		else if (context->getResponse()->ifTransferEncoding(HttpResponse::TransferEncoding::CHUNKED))
		{
			_log.debug("transfer-encodinfg chunked");
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
//					HttpResponse(400)
//						<< HttpHeader("Connection", "Close")
//						<< "Wrong head of chunk for chunked Transfer-Encoding" << "\r\n"
//						>> *connection;

					_log.debug("Wrong head of chunk for chunked Transfer-Encoding");
					connection->setTtl(std::chrono::milliseconds(50));
					return true;
				}

				// Слишком большой чанк
				if (size > 1ull << 22) // 4Mb
				{
//					HttpResponse(500)
//						<< HttpHeader("Connection", "Close")
//						<< "Too large chunk for chunked Transfer-Encoding" << "\r\n"
//						>> *connection;

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
				if (strncmp(endHeader + 2 + size, "\r\n", 2))
				{
//					HttpResponse(400)
//						<< HttpHeader("Connection", "Close")
//						<< "Wrong end of chunk for chunked Transfer-Encoding" << "\r\n"
//						>> *connection;

					_log.debug("Wrong end of chunk for chunked Transfer-Encoding");
					connection->setTtl(std::chrono::milliseconds(50));
					return true;
				}

				context->getResponse()->write(endHeader + 2, size);

				connection->skip(endHeader + 2 + size + 2 - connection->dataPtr());

				// Чанк нулевого размера
				if (size == 0)
				{
					break;
				}

				// Данных больше не будет
				if (connection->noRead())
				{
					_log.debug("Incomplete body of response");
					connection->setTtl(std::chrono::milliseconds(50));
					return true;
				}
			}
		}
		else
		{
			_log.debug("until close");
			context->getResponse()->write(connection->dataPtr(), connection->dataLen());

			connection->skip(connection->dataLen());

			if (!connection->noRead())
			{
				_log.debug("Not read all request body yet (read %zu)", context->getResponse()->dataLen());
				break;
			}
		}

		_log.debug("RESPONSE: %u '%s' %zu",
			context->getResponse()->statusCode(),
			context->getResponse()->statusMessage().c_str(),
			context->getResponse()->hasContentLength() ? context->getResponse()->contentLength() : 0
		);

		connection->onComplete();

		connection->resetContext();
		n++;
	}

	_log.debug("Processed %d response, remain %zu bytes", n, connection->dataLen());

	return false;
}
