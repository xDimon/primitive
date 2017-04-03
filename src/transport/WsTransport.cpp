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

// WsTransport.cpp


#include <sstream>
#include <memory>
#include "../net/ConnectionManager.hpp"
#include "../net/TcpConnection.hpp"
#include "../server/Server.hpp"
#include "../utils/Base64.hpp"
#include "../utils/Packet.hpp"
#include "../utils/SHA1.hpp"
#include "../utils/Time.hpp"
#include "websocket/WsContext.hpp"
#include "websocket/WsFrame.hpp"
#include "WsTransport.hpp"

WsTransport::WsTransport(std::string host, uint16_t port)
: Log("WsTransport")
, _host(host)
, _port(port)
, _acceptor(TcpAcceptor::Ptr())
{
	log().debug("Create transport 'WsTransport({}:{})'", _host, _port);
}

WsTransport::~WsTransport()
{
}

bool WsTransport::enable()
{
	if (!_acceptor.expired())
	{
		return true;
	}
	log().debug("Enable transport 'WsTransport({}:{})'", _host, _port);
	try
	{
		Transport::Ptr transport = ptr();

		auto acceptor = std::shared_ptr<Connection>(new TcpAcceptor(transport, _host, _port));

		_acceptor = acceptor->ptr();

		ConnectionManager::add(_acceptor.lock());

		return true;
	}
	catch(std::runtime_error exception)
	{
		log().debug("Can't create Acceptor for transport 'WsTransport({}:{})': {}", _host, _port, exception.what());

		return false;
	}
}

bool WsTransport::disable()
{
	if (_acceptor.expired())
	{
		return true;
	}
	log().debug("Disable transport 'WsTransport({}:{})'", _host, _port);

	ConnectionManager::remove(_acceptor.lock());

	_acceptor.reset();

	return true;
}

bool WsTransport::processing(std::shared_ptr<Connection> connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	if (!connection->getContext())
	{
		auto context = WsContext::Ptr(new WsContext());

		connection->setContext(context);
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
			log().debug("Not anough data for read headers ({} bytes)", connection->dataLen());
			return true;
		}

		if (!endHeaders || (endHeaders - connection->dataPtr()) > (1 << 12))
		{
			log().debug("Headers part of request too large ({} bytes)", endHeaders - connection->dataPtr());

			std::stringstream ss;
			ss << "HTTP/1.0 400 Bad request\r\n"
				<< "Server: " << Server::httpName() << "\r\n"
				<< "Date: " << Time::httpDate() << "\r\n"
				<< "X-Transport: websocket\r\n"
				<< "\r\n"
				<< "Headers data too large" << "\r\n";

			// Формируем пакет
			Packet response(ss.str().c_str(), ss.str().size());

			// Отправляем
			connection->write(response.data(), response.size());

			connection->close();

			return true;
		}

		size_t headersSize = endHeaders - connection->dataPtr() + 4;

		log().debug("Read {} bytes of request headers", headersSize);

		try
		{
			// Читаем запрос
			auto request = std::make_shared<HttpRequest>(connection->dataPtr(), connection->dataPtr() + headersSize);

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

			std::stringstream ss;
			ss << "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
				<< "Server: " << Server::httpName() << "\r\n"
				<< "Date: " << Time::httpDate() << "\r\n"
				<< "X-Transport: websocket\r\n"
				<< "Upgrade: websocket\r\n"
				<< "Connection: Upgrade\r\n"
				<< "Sec-WebSocket-Accept: " << acceptKey << "\r\n"
				<< "\r\n";

			// Формируем пакет
			Packet response(ss.str().c_str(), ss.str().size());

			// Отправляем
			connection->write(response.data(), response.size());

			context->setEstablished();

			context->getRequest().reset();

			//
			// http://learn.javascript.ru/websockets#описание-фрейма
			//
		}
		catch (std::runtime_error &exception)
		{
			std::stringstream ss;
			ss << "HTTP/1.0 400 Bad request\r\n"
				<< "Server: " << Server::httpName() << "\r\n"
				<< "Date: " << Time::httpDate() << "\r\n"
				<< "X-Transport: websocket\r\n"
				<< "\r\n"
				<< exception.what() << "\r\n";

			// Формируем пакет
			Packet response(ss.str().c_str(), ss.str().size());

			// Отправляем
			connection->write(response.data(), response.size());

			connection->close();

			return true;
		}

		// Пропускаем байты заголовка
		connection->skip(headersSize);
	}

	int n = 0;

	// Цикл извлечения фреймов
	for (;;)
	{
		// Пробуем читать новый фрейм
		if (!context->getFrame())
		{
			// Недостаточно данных для получения размера заголовка фрейма
			if (connection->dataLen() < 2)
			{
				log().debug("Not anough data for calc frame header size ({} < 2)", connection->dataLen());
				break;
			}

			// Недостаточно данных для чтения заголовка фрейма
			size_t headerSize = WsFrame::calcHeaderSize(connection->dataPtr());
			if (connection->dataLen() < headerSize)
			{
				log().debug("Not anough data for get frame header ({} < {})", connection->dataLen(), headerSize);
				break;
			}

			auto frame = WsFrame::Ptr(new WsFrame(connection->dataPtr(), connection->dataPtr() + connection->dataLen()));

			log().debug("Read {} bytes of frame header. Size of data: {} bytes", headerSize, frame->contentLength());

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
				log().debug("Not anough data for read frame body ({} < {})", context->getFrame()->dataLen(), context->getFrame()->contentLength());
				break;
			}
		}

		context->getFrame()->applyMask();


		if (context->getFrame()->opcode() == WsFrame::Opcode::Text)
		{
			log().debug("FRAME-TEXT: \"{}\" {} bytes", context->getFrame()->dataPtr(), context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Text, context->getFrame()->dataPtr(), context->getFrame()->dataLen());
			context->getFrame().reset();
			n++;
			continue;
		}
		else if (context->getFrame()->opcode() == WsFrame::Opcode::Binary)
		{
			log().debug("FRAME-BINARY: {} bytes", context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Text, "Received binary data", 21);
			context->getFrame().reset();
			n++;
			continue;
		}
		else if (context->getFrame()->opcode() == WsFrame::Opcode::Ping)
		{
			WsFrame::send(connection, WsFrame::Opcode::Pong, context->getFrame()->dataPtr(), context->getFrame()->dataLen());
			log().debug("FRAME-PING: {} bytes", context->getFrame()->dataLen());
			context->getFrame().reset();
			n++;
			continue;
		}
		else if (context->getFrame()->opcode() == WsFrame::Opcode::Close)
		{
			log().debug("FRAME-CLOSE: {} bytes", context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Close, "Bye!", 4);
			context.reset();
			n++;
			break;
		}
		else
		{
			log().debug("FRAME-UNSUPPORTED(#{}): {} bytes", static_cast<int>(context->getFrame()->opcode()), context->getFrame()->dataLen());
			WsFrame::send(connection, WsFrame::Opcode::Close, "Unsupported opcode", 21);
			context.reset();
			n++;
			break;
		}
	}

	log().debug("Processed {} request, remain {} bytes", n, connection->dataLen());

	return true;
}
