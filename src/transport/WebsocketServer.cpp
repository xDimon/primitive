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
// File created on: 2017.03.29

// WebsocketServer.cpp


#include <sstream>
#include <memory>
#include <cstring>
#include "../net/ConnectionManager.hpp"
#include "../net/TcpConnection.hpp"
#include "../server/Server.hpp"
#include "../utils/Base64.hpp"
#include "../utils/hash/SHA1.hpp"
#include "websocket/WsContext.hpp"
#include "WebsocketServer.hpp"
#include "http/HttpResponse.hpp"

REGISTER_TRANSPORT(websocket, WebsocketServer);

bool WebsocketServer::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
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

	if (!context->established())
	{
		if (connection->dataLen() >= 23)
		{
			if (memcmp(connection->dataPtr(), "<policy-file-request/>\0", 23) == 0)
			{
				connection->skip(23);

				const std::string& policyFile = R"(<?xml version="1.0"?>
<!DOCTYPE cross-domain-policy SYSTEM "http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd">
<cross-domain-policy>
  <allow-access-from domain="*" secure="false" to-ports="*"/>
  <allow-access-from domain="*" secure="true" to-ports="*"/>
  <site-control permitted-cross-domain-policies="all"/>
</cross-domain-policy>
)";

				connection->write(policyFile.c_str(), policyFile.length());

				connection->close();
				connection->resetContext();
				connection->setTtl(std::chrono::milliseconds(50));

				_log.debug("Sent policy file");

				return true;
			}

			if (
				strncasecmp(connection->dataPtr(), "GET", 3) != 0 &&
				strncasecmp(connection->dataPtr(), "POST", 4) != 0
			)
			{
				HttpResponse(400)
					<< HttpHeader("Connection", "Close")
					<< "Bad request ← Bad request line ← Bad method ← Unknown method" << "\r\n"
					>> *connection;

				connection->setTtl(std::chrono::milliseconds(50));
				connection->close();
				connection->resetContext();

				_log.debug("Bad initial request");

				return true;
			}
		}

		// Проверка готовности заголовков
		auto endHeaders = static_cast<const char *>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

		if (endHeaders == nullptr && connection->dataLen() < (1 << 12))
		{
			connection->setTtl(std::chrono::seconds(5));

			_log.debug("Not anough data for read headers (%zu bytes)", connection->dataLen());
			return true;
		}

		if (endHeaders == nullptr || (endHeaders - connection->dataPtr()) > (1 << 12))
		{
			connection->skip(endHeaders - connection->dataPtr());

			_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

			HttpResponse(400, "Bad request")
				<< HttpHeader("Connection", "close")
				<< "Headers data too large\r\n"
				>> *connection;

			connection->resetContext();
			connection->setTtl(std::chrono::milliseconds(50));

			return true;
		}

		size_t headersSize = endHeaders - connection->dataPtr() + 4;

		_log.debug("Read %zu bytes of request headers", headersSize);

		try
		{
			std::shared_ptr<HttpRequest> request;

			// Читаем запрос
			try
			{
				request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);
			}
			catch (...)
			{
				connection->skip(headersSize);
				throw;
			}

			// Пропускаем байты заголовка
			connection->skip(headersSize);

			context->setRequest(request);

			// Допустим только GET
			if (context->getRequest()->method() != HttpRequest::Method::GET)
			{
				throw std::runtime_error("Unsupported method");
			}

			// Проверка заголовков
			if (
				strcasecmp(request->getHeader("Connection").c_str(), "Upgrade") ||
				strcasecmp(request->getHeader("Upgrade").c_str(), "websocket") ||
				request->getHeader("Sec-WebSocket-Key").empty()
			)
			{
				throw std::runtime_error("Bad headers");
			}

			const auto& wsKey = request->getHeader("Sec-WebSocket-Key");

			// (echo -n "$1"; echo -n '258EAFA5-E914-47DA-95CA-C5AB0DC85B11') | sha1sum | xxd -r -p | base64
			auto acceptKey = Base64::encode(SHA1::encode_bin(wsKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));

			auto handler = getHandler(context->getRequest()->uri().path());
			if (!handler)
			{
				HttpResponse(404, "WebSocket Upgrade Failure")
					<< HttpHeader("X-ServerTransport", "websocket", true)
					<< HttpHeader("Connection", "Close")
					<< "Not found service-handler for uri " << context->getRequest()->uri().path() << "\r\n"
					>> *connection;

				connection->resetContext();
				connection->setTtl(std::chrono::milliseconds(50));

				return true;
			}

			HttpResponse(101, "Web Socket Protocol Handshake")
				<< HttpHeader("X-ServerTransport", "websocket", true)
				<< HttpHeader("Upgrade", "websocket")
				<< HttpHeader("Connection", "Upgrade")
				<< HttpHeader("Sec-WebSocket-Accept", acceptKey)
				>> *connection;

			if (!handler)
			{
				std::string msg("##Not found service-handler for uri ");
				msg += context->getRequest()->uri().path();
				uint16_t code = htobe16(1008); // Policy Violation
				memcpy(const_cast<char *>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());

				connection->resetContext();
				connection->setTtl(std::chrono::milliseconds(50));

				return true;
			}

			context->setHandler(handler);

			context->setEstablished();

			connection->setTtl(std::chrono::seconds(10));
		}
		catch (std::runtime_error &exception)
		{
			HttpResponse(400)
				<< HttpHeader("X-ServerTransport", "websocket", true)
				<< HttpHeader("Connection", "Close")
				<< exception.what() << "\r\n"
				>> *connection;

			connection->resetContext();
			connection->setTtl(std::chrono::milliseconds(50));

			return true;
		}
	}

	int n = 0;

	// Цикл извлечения фреймов
	for (;;)
	{
		// http://learn.javascript.ru/websockets#описание-фрейма

		// Пробуем читать новый фрейм
		if (!context->getFrame())
		{
			// Недостаточно данных для получения размера заголовка фрейма
			if (connection->dataLen() < 2)
			{
				if (connection->dataLen() > 0)
				{
					_log.debug("Not anough data for calc frame header size (%zu < 2)", connection->dataLen());
					connection->setTtl(std::chrono::seconds(60));
				}
				else
				{
					_log.debug("No more data");
					connection->setTtl(std::chrono::seconds(900));
				}
				break;
			}

			// Недостаточно данных для чтения заголовка фрейма
			size_t headerSize = WsFrame::calcHeaderSize(connection->dataPtr());
			if (connection->dataLen() < headerSize)
			{
				_log.debug("Not anough data for get frame header (%zu < %zu)", connection->dataLen(), headerSize);
				connection->setTtl(std::chrono::seconds(60));
				break;
			}

			auto frame = std::make_shared<WsFrame>(connection->dataPtr(), connection->dataPtr() + connection->dataLen());
			_log.debug("Read %zu bytes of frame header. Size of data: %zu bytes", headerSize, frame->contentLength());

			context->setFrame(frame);

			// Пропускаем байты заголовка фрейма
			connection->skip(headerSize);
		}

		// Читаем данные фрейма
		if (context->getFrame()->contentLength() >= context->getFrame()->dataLen())
		{
			size_t len = std::min(context->getFrame()->contentLength() - context->getFrame()->dataLen(), connection->dataLen());

			context->getFrame()->write(connection->dataPtr(), len);

			connection->skip(len);

			if (context->getFrame()->contentLength() > context->getFrame()->dataLen())
			{
				_log.debug("Not anough data for read frame body (%zu < %zu)", context->getFrame()->dataLen(), context->getFrame()->contentLength());
				connection->setTtl(std::chrono::seconds(60));
				break;
			}
		}

		context->getFrame()->applyMask();

		switch (context->getFrame()->opcode())
		{
			case WsFrame::Opcode::Text:
			case WsFrame::Opcode::Binary:
			{
				_log.debug("TEXT-FRAME: \"%s\" %zu bytes", context->getFrame()->dataPtr(), context->getFrame()->dataLen());

				context->setTransmitter(
					std::make_shared<Transport::Transmitter>(
						[wp = std::weak_ptr<TcpConnection>(connection), opcode = context->getFrame()->opcode()]
							(const char* data, size_t size, const std::string&, bool close) {
							auto connection = std::dynamic_pointer_cast<TcpConnection>(wp.lock());
							if (!connection)
							{
								return;
							}
							WsFrame::send(connection, opcode, data, size);
							if (close)
							{
								std::string msg("##Bye!\n");
								uint16_t code = htobe16(1000); // Normal Closure
								memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));

								WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
								connection->setTtl(std::chrono::milliseconds(50));
								connection->close();
								connection->resetContext();
							}
							ConnectionManager::watch(connection);
						}
					)
				);

				connection->setTtl(std::chrono::seconds(900));

				context->handle();

				context->resetFrame();

				n++;
				continue;
			}
			case WsFrame::Opcode::Ping:
			{
				_log.debug("PING-FRAME: %zu bytes", context->getFrame()->dataLen());

				WsFrame::send(connection, WsFrame::Opcode::Pong, context->getFrame()->dataPtr(), context->getFrame()->dataLen());

				context->resetFrame();

				connection->setTtl(std::chrono::seconds(900));

				n++;
				continue;
			}
			case WsFrame::Opcode::Close:
			{
				_log.debug("CLOSE-FRAME: %zu bytes", context->getFrame()->dataLen());

				std::string msg("##Bye!\n");
				uint16_t code = htobe16(1000); // Normal Closure
				memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
				connection->setTtl(std::chrono::milliseconds(50));
				connection->close();
				connection->resetContext();
				n++;
				goto end;
			}
			default:
				_log.debug("UNSUPPORTED-FRAME(#%d): %zu bytes", static_cast<int>(context->getFrame()->opcode()), context->getFrame()->dataLen());

				std::string msg("##Unsupported opcode\n");
				uint16_t code = htobe16(1003); // Unsupported data
				memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
				connection->setTtl(std::chrono::milliseconds(50));
				connection->close();
				connection->resetContext();
				n++;
				goto end;
		}
	}

	end:

	_log.debug("Processed %d request, remain %zu bytes", n, connection->dataLen());

	return true;
}

void WebsocketServer::bindHandler(const std::string& selector, const std::shared_ptr<ServerTransport::Handler>& handler)
{
	if (_handlers.find(selector) != _handlers.end())
	{
		throw std::runtime_error("Handler already set early");
	}

	_handlers.emplace(selector, handler);
}

std::shared_ptr<ServerTransport::Handler> WebsocketServer::getHandler(const std::string& subject_)
{
	auto subject = subject_;
	do
	{
		auto i = _handlers.upper_bound(subject);

		if (i != _handlers.end())
		{
//			_log.debug(">>> '%s' upper_bound: '%s'", subject.c_str(), i->first.c_str());
			if (i->first == subject)
			{
//				_log.debug(">>> DONE: found '%s'", i->first.c_str());
				return i->second;
			}
		}
//		_log.debug(">>> '%s' upper_bound: end", subject.c_str());

		if (i == _handlers.begin())
		{
//			_log.debug(">>> '%s' upper_bound: begin", subject.c_str());
//			_log.debug(">>> DONE: not found");
			break;
		}

		--i;
//		_log.debug(">>> '%s' previously:  '%s'", subject.c_str(), i->first.c_str());

		size_t len = std::min(subject.length(), i->first.length());

		int cmpres = std::strncmp(i->first.c_str(), subject.c_str(), len);
		if (cmpres == 0)
		{
//			_log.debug(">>> DONE: found '%s'", i->first.c_str());
			return i->second;
		}
		if (cmpres > 0)
		{
//			_log.debug(">>> DONE: not found");
			return nullptr;
		}

		subject.resize(--len);
//		_log.debug(">>> sbj decrease to '%s'", subject.c_str(), i->first.c_str());
	}
	while (!subject.empty());

	return nullptr;
}
