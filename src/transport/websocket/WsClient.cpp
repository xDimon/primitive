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
// File created on: 2017.10.06

// WsClient.cpp


#include <string.h>
#include "WsClient.hpp"
#include "../../net/TcpConnection.hpp"
#include "../http/HttpContext.hpp"
#include "WsContext.hpp"
#include "../../utils/encoding/Base64.hpp"
#include "../../utils/hash/SHA1.hpp"

bool WsClient::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for WsClient");
	}

	if (!connection->getContext())
	{
		connection->setContext(std::make_shared<WsContext>(connection));
	}
	auto context = std::dynamic_pointer_cast<WsContext>(connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context-type for this transport");
	}

	if (context->established())
	{
		throw std::runtime_error("Websocket communication already established");
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
			return true;
		}

		if (endHeaders == nullptr  || (endHeaders - connection->dataPtr()) > (1ll << 12))
		{
			_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

			connection->setTtl(std::chrono::milliseconds(50));
			return true;
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
			return true;
		}

		// Пропускаем байты заголовка
		connection->skip(headersSize);
	}

	auto response = context->getResponse();

	// Допустим только ответ 101
	if (response->statusCode() != 101)
	{
		_log.debug(("Fail Handshake: " + std::to_string(response->statusCode()) + " " + response->statusMessage()).c_str());
		connection->setTtl(std::chrono::milliseconds(50));
		return true;
	}

	// Проверка заголовков
	if (
		strcasestr(response->getHeader("Connection").c_str(), "Upgrade") == nullptr ||
		strcasestr(response->getHeader("Upgrade").c_str(), "websocket") == nullptr ||
		response->getHeader("Sec-WebSocket-Accept").empty()
	)
	{
		_log.debug("Bad headers");
		connection->setTtl(std::chrono::milliseconds(50));
		return true;
	}

	const auto& serverAcceptKey = response->getHeader("Sec-WebSocket-Accept");

	// (echo -n "$1"; echo -n '258EAFA5-E914-47DA-95CA-C5AB0DC85B11') | sha1sum | xxd -r -p | base64
	const auto acceptKey = Base64::encode(SHA1::encode_bin(context->key() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));

	if (acceptKey != serverAcceptKey)
	{
		_log.debug("No match accept key");
		connection->setTtl(std::chrono::milliseconds(50));
		return true;
	}

	context->setHandler(_handler);

	context->setEstablished();

	return true;
}
