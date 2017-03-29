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
#include "websocket/WsContext.hpp"
#include "../utils/Packet.hpp"
#include "../net/ConnectionManager.hpp"
#include "../net/TcpConnection.hpp"
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

	int n = 0;

	// Цикл обработки запросов
	for (;;)
	{
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
				break;
			}

			if (!endHeaders || (endHeaders - connection->dataPtr()) > (1 << 12))
			{
				log().debug("Headers part of request too large ({} bytes)", endHeaders - connection->dataPtr());

				std::stringstream ss;
				ss << "HTTP/1.0 400 Bad request\r\n"
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
				//
				// http://learn.javascript.ru/websockets#описание-фрейма
				//
			}
			catch (std::runtime_error &exception)
			{
				std::stringstream ss;
				ss << "HTTP/1.0 400 Bad request\r\n"
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

		//// Читаем фрейм
		// Заголовок
		// Тело

		//// Обрабатываем


		//// Отправляем фрейм

//		log().debug("REQUEST: {} {} {}", context->getRequest()->method_s(), context->getRequest()->uri_s(), context->getRequest()->hasContentLength() ? context->getRequest()->contentLength() : 0);

		context->getRequest().reset();

		// Формируем пакет
//		Packet response(ss.str().c_str(), ss.str().size());

		// Отправляем
//		connection->write(response.data(), response.size());

		connection->close();

		n++;
	}

	log().debug("Processed {} request, remain {} bytes", n, connection->dataLen());

	return false;
}
