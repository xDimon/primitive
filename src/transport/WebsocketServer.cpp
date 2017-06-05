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
#include "../utils/SHA1.hpp"
#include "../utils/Time.hpp"
#include "websocket/WsContext.hpp"
#include "WebsocketServer.hpp"
#include "http/HttpResponse.hpp"

REGISTER_TRANSPORT(websocket, WebsocketServer);

bool WebsocketServer::processing(std::shared_ptr<Connection> connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	if (!connection->getContext())
	{
		connection->setContext(std::make_shared<WsContext>(connection)->ptr());
	}
	auto context = std::dynamic_pointer_cast<WsContext>(connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context-type for this transport");
	}

	if (!context->established())
	{
		// Проверка готовности заголовков
		auto endHeaders = static_cast<const char *>(memmem(connection->dataPtr(), connection->dataLen(), "\r\n\r\n", 4));

		if (!endHeaders && connection->dataLen() < (1 << 12))
		{
			_log.debug("Not anough data for read headers (%zu bytes)", connection->dataLen());
			return true;
		}

		if (!endHeaders || (endHeaders - connection->dataPtr()) > (1 << 12))
		{
			_log.debug("Headers part of request too large (%zu bytes)", endHeaders - connection->dataPtr());

			HttpResponse(400)
				<< HttpHeader("Connection", "close")
				<< "Headers data too large\n"
				>> *connection;

			return true;
		}

		size_t headersSize = endHeaders - connection->dataPtr() + 4;

		_log.debug("Read %zu bytes of request headers", headersSize);

		try
		{
			// Читаем запрос
			auto request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);

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
				request->getHeader("Connection") != "Upgrade" ||
				request->getHeader("Upgrade") != "websocket" ||
				request->getHeader("Sec-WebSocket-Key") == ""
			)
			{
				throw std::runtime_error("Bad headers");
			}

			auto& wsKey = request->getHeader("Sec-WebSocket-Key");

			// (echo -n "$1"; echo -n '258EAFA5-E914-47DA-95CA-C5AB0DC85B11') | sha1sum | xxd -r -p | base64
			auto acceptKey = Base64::encode(SHA1::encode_bin(wsKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));

			auto handler = getHandler(context->getRequest()->uri().path());
			if (!handler)
			{
				HttpResponse(404, "WebSocket Upgrade Failure")
					<< HttpHeader("X-ServerTransport", "websocket", true)
					<< HttpHeader("Connection", "Close")
					<< "Not found service-Handler for uri " << context->getRequest()->uri().path()
					>> *connection;

				connection->close();
				context.reset();
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
				std::string msg("##Not found service-Handler for uri ");
				msg += context->getRequest()->uri().path();
				uint16_t code = htobe16(1008); // Policy Violation
				memcpy(const_cast<char *>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
				context.reset();
				connection->close();
				return true;
			}

			context->getRequest().reset();

			context->setHandler(std::move(handler));

			context->setEstablished();
		}
		catch (std::runtime_error &exception)
		{
			HttpResponse(400)
				<< HttpHeader("X-ServerTransport", "websocket", true)
				<< HttpHeader("Connection", "Close")
				<< exception.what()
				>> *connection;

			connection->close();

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
				if (connection->dataLen())
				{
					_log.debug("Not anough data for calc frame header size (%zu < 2)", connection->dataLen());
				}
				else
				{
					_log.debug("No more data");
				}
				break;
			}

			// Недостаточно данных для чтения заголовка фрейма
			size_t headerSize = WsFrame::calcHeaderSize(connection->dataPtr());
			if (connection->dataLen() < headerSize)
			{
				_log.debug("Not anough data for get frame header (%zu < %zu)", connection->dataLen(), headerSize);
				break;
			}

			auto frame = std::shared_ptr<WsFrame>(new WsFrame(connection->dataPtr(), connection->dataPtr() + connection->dataLen()));

			_log.debug("Read %zu bytes of frame header. Size of data: %zu bytes", headerSize, frame->contentLength());

			context->setFrame(frame);

			// Пропускаем байты заголовка фрейма
			connection->skip(headerSize);
		}

		// Читаем данные фрейма
		if (context->getFrame()->contentLength() > context->getFrame()->dataLen())
		{
			size_t len = std::min(context->getFrame()->contentLength() - context->getFrame()->dataLen(), connection->dataLen());

			context->getFrame()->write(connection->dataPtr(), len);

			connection->skip(len);

			if (context->getFrame()->contentLength() > context->getFrame()->dataLen())
			{
				_log.debug("Not anough data for read frame body (%zu < %zu)", context->getFrame()->dataLen(), context->getFrame()->contentLength());
				break;
			}
		}

		context->getFrame()->applyMask();

		if (context->getFrame()->opcode() == WsFrame::Opcode::Text || context->getFrame()->opcode() == WsFrame::Opcode::Binary)
		{
			ServerTransport::Transmitter transmitter =
				[&connection,opcode = context->getFrame()->opcode()]
					(const char*data, size_t size, bool close){
					WsFrame::send(connection, opcode, data, size);
					if (close)
					{
						std::string msg("##Bye!");
						uint16_t code = htobe16(1000); // Normal Closure
						memcpy(const_cast<char *>(msg.data()), &code, sizeof(code));

						WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
						connection->close();
					}
				};

			context->handle(context->getFrame()->dataPtr(), context->getFrame()->dataLen(), transmitter);
			context->getFrame().reset();
			n++;
			continue;
		}
//		else if (context->getFrame()->opcode() == WsFrame::Opcode::Text)
//		{
//			_log.debug("TEXT-FRAME: \"%s\" %zu bytes", context->getFrame()->dataPtr(), context->getFrame()->dataLen());
//			WsFrame::send(connection, WsFrame::Opcode::Text, context->getFrame()->dataPtr(), context->getFrame()->dataLen());
//			context->getFrame().reset();
//			n++;
//			continue;
//		}
//		else if (context->getFrame()->opcode() == WsFrame::Opcode::Binary)
//		{
//			_log.debug("BINARY-FRAME: %zu bytes", context->getFrame()->dataLen());
//			WsFrame::send(connection, WsFrame::Opcode::Text, "Received binary data", 21);
//			context->getFrame().reset();
//			n++;
//			continue;
//		}
		else if (context->getFrame()->opcode() == WsFrame::Opcode::Ping)
		{
			WsFrame::send(connection, WsFrame::Opcode::Pong, context->getFrame()->dataPtr(), context->getFrame()->dataLen());
			_log.debug("PING-FRAME: %zu bytes", context->getFrame()->dataLen());
			context->getFrame().reset();
			n++;
			continue;
		}
		else if (context->getFrame()->opcode() == WsFrame::Opcode::Close)
		{
			_log.debug("CLOSE-FRAME: %zu bytes", context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Close, "Bye!", 4);
			context.reset();
			connection->close();
			n++;
			break;
		}
		else
		{
			_log.debug("UNSUPPORTED-FRAME(#%d): %zu bytes", static_cast<int>(context->getFrame()->opcode()), context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Close, "Unsupported opcode", 21);
			context.reset();
			n++;
			break;
		}
	}

	_log.debug("Processed %d request, remain %zu bytes", n, connection->dataLen());

	return true;
}

void WebsocketServer::bindHandler(const std::string& selector, ServerTransport::Handler handler)
{
	if (_handlers.find(selector) != _handlers.end())
	{
		throw std::runtime_error("Handler already set early");
	}

	_handlers.emplace(selector, handler);
}

ServerTransport::Handler WebsocketServer::getHandler(std::string subject)
{
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
		else if (cmpres > 0)
		{
//			_log.debug(">>> DONE: not found");
			return nullptr;
		}

		subject.resize(--len);
//		_log.debug(">>> sbj decrease to '%s'", subject.c_str(), i->first.c_str());
	}
	while (subject.length());

	return nullptr;
}
