// Copyright © 2017-2019 Dmitriy Khaustov
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
// File created on: 2017.03.09

// TcpAcceptor.cpp


#include "TcpAcceptor.hpp"

#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include "ConnectionManager.hpp"
#include "TcpConnection.hpp"
#include "../utils/Daemon.hpp"

TcpAcceptor::TcpAcceptor(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port)
: Acceptor(transport)
, _host(host)
, _port(port)
{
	// Создаем сокет
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == -1)
	{
		throw std::runtime_error("Can't create socket");
	}

	_name = "TcpAcceptor[" + std::to_string(_sock) + "][" + host + ":" + std::to_string(port) + "]";

	const int val = 1;

	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	sockaddr_in servaddr{};
	socklen_t addrlen = sizeof(sockaddr_in);

	// Инициализируем структуру нулями
	memset(&servaddr, 0, sizeof(servaddr));

	// Задаем семейство сокетов (IPv4)
	servaddr.sin_family = AF_INET;

	// Задаем хост
	if (!_host.empty())
	{
		if (inet_aton(_host.c_str(), &servaddr.sin_addr) == 0)
		{
			_log.debug("Can't convert host to binary IPv4 address (error '%s'). I'll use universal address.", strerror(errno));

			// Задаем хост (INADDR_ANY - универсальный)
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}
	else
	{
		// Задаем хост (INADDR_ANY - универсальный)
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	// Задаем порт
	servaddr.sin_port = htons(_port);

	// Связываем сокет с локальным адресом протокола
	if (bind(_sock, (struct sockaddr *) &servaddr, addrlen) != 0)
	{
		throw std::runtime_error(std::string("Can't bind socket ← ") + strerror(errno));
	}

	// Преобразуем сокет в пассивный (слушающий) и устанавливаем длину очереди соединений
	if (listen(_sock, 16) != 0)
	{
		throw std::runtime_error(std::string("Can't listen port ← ") + strerror(errno));
	}

	// Включем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	_log.debug("%s created", name().c_str());
}

TcpAcceptor::~TcpAcceptor()
{
	_log.debug("%s destroyed", name().c_str());
}

void TcpAcceptor::watch(epoll_event &ev)
{
	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	ev.events |= EPOLLERR;
	ev.events |= EPOLLIN;
}

bool TcpAcceptor::processing()
{
	_log.debug("Processing on %s", name().c_str());

	std::lock_guard<std::mutex> guard(_mutex);
	for (;;)
	{
		if (Daemon::shutingdown())
		{
			_log.debug("Interrupt processing on %s (shutdown)", name().c_str());
			ConnectionManager::remove(ptr());
			return false;
		}

		if (wasFailure())
		{
			_closed = true;

			_log.debug("End processing on %s (was failure)", name().c_str());
			ConnectionManager::remove(ptr());
			throw std::runtime_error("Error on TcpAcceptor");
		}

		sockaddr_in cliaddr{};
		socklen_t clilen = sizeof(cliaddr);
		memset(&cliaddr, 0, clilen);

		int sock = ::accept(fd(), (sockaddr *)&cliaddr, &clilen);
		if (sock == -1)
		{
			// Вызов прерван сигналом - повторяем
			if (errno == EINTR)
			{
				continue;
			}

			// Нет подключений - на этом все
			if (errno == EAGAIN)
			{
				_log.debug("End processing on %s (no more accept)", name().c_str());
				return true;
			}

			// Ошибка установления соединения
			_log.debug("End processing on %s (accept error: %s)", name().c_str(), strerror(errno));
			return false;
		}

		_log.debug("%s accept [%u]", name().c_str(), sock);

		// Перевод сокета в неблокирующий режим
		int val = fcntl(sock, F_GETFL, 0);
		fcntl(sock, F_SETFL, val | O_NONBLOCK);

		try
		{
			createConnection(sock, cliaddr);
		}
		catch (const std::exception& exception)
		{
			shutdown(sock, SHUT_RDWR);
			_log.info("%s close [%u]", name().c_str(), sock);
			::close(sock);
		}
	}
}

void TcpAcceptor::createConnection(int sock, const sockaddr_in &cliaddr)
{
	auto transport = std::dynamic_pointer_cast<ServerTransport>(_transport.lock());
	if (!transport)
	{
		return;
	}

	auto newConnection = std::make_shared<TcpConnection>(transport, sock, cliaddr, false);

	newConnection->setTtl(std::chrono::seconds(5));

	ConnectionManager::add(newConnection->ptr());

	transport->metricConnectCount->addValue();
}
