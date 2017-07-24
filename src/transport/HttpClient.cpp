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
// File created on: 2017.06.06

// HttpClient.cpp


#include <string.h>
#include "HttpClient.hpp"
#include "../net/TcpConnection.hpp"
#include "http/HttpContext.hpp"

bool HttpClient::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	int n = 0;

	// Цикл обработки ответа
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

		// Незавершенного ответа нет
		if (!context->getResponse())
		{
			// Проверка готовности заголовков
			auto endHeaders = static_cast<const char*>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

			if (endHeaders == nullptr && connection->dataLen() < (1 << 12))
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

			if (endHeaders == nullptr  || (endHeaders - connection->dataPtr()) > (1 << 12))
			{
				_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

				connection->close(); // TODO обработать ошибку

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
			catch (std::runtime_error& exception)
			{
				connection->close(); // TODO обработать ошибку

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
					_log.debug("Not anough data for read request body ({%zu < %zu)",
								context->getResponse()->dataLen(), context->getResponse()->contentLength());
					break;
				}
			}
		}
		else
		{
			context->getResponse()->write(connection->dataPtr(), connection->dataLen());

			connection->skip(connection->dataLen());

			if (!connection->noRead())
			{
				_log.debug("Not read all request body yet (read %zu)", context->getResponse()->dataLen());
				break;
			}
		}

		_log.debug("REQUEST: %u '%s' %zu",
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
