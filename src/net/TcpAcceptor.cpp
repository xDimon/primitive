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
// File created on: 2017.03.09

// TcpAcceptor.cpp


#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <cstring>
#include <zconf.h>
#include "../log/Log.hpp"

#include "TcpAcceptor.hpp"
#include "ConnectionManager.hpp"
#include "TcpConnection.hpp"

TcpAcceptor::TcpAcceptor(std::string host, std::uint16_t port)
: ConnectionBase()
, _host(host)
, _port(port)
{
	// Создаем сокет
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == -1)
	{
		throw std::runtime_error("Can't create socket");
	}

	const int val = 1;

	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	sockaddr_in servaddr;
	socklen_t addrlen = sizeof(sockaddr_in);

	// Инициализируем структуру нулями
	memset(&servaddr, 0, sizeof(servaddr));

	// Задаем семейство сокетов (IPv4)
	servaddr.sin_family = AF_INET;

	// Задаем хост
	if (_host.length())
	{
		if (!inet_aton(_host.c_str(), &servaddr.sin_addr))
		{
			Log().debug("Can't convert host to binary IPv4 address (error '{}'). I'll use universal address.", strerror(errno));

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
		close(_sock);
		throw std::runtime_error("Can't bind socket");
	}

	// Преобразуем сокет в пассивный (слушающий) и устанавливаем длину очереди соединений
	if (listen(_sock, 16) != 0)
	{
		close(_sock);
		throw std::runtime_error("Can't listen port");
	}

	// Включем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	Log().debug("Create {}", name());
}

TcpAcceptor::~TcpAcceptor()
{
	Log().debug("Destroy {}", name());
}

const std::string& TcpAcceptor::name()
{
	if (_name.empty())
	{
		std::stringstream ss;
		ss << "TcpAcceptor [" << _sock << "] [" << _host << ":" << _port << "]";
		_name = ss.str();
	}
	return _name;
}

void TcpAcceptor::watch(epoll_event &ev)
{
	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLERR;
	ev.events |= EPOLLIN;
}

bool TcpAcceptor::processing()
{
	Log().debug("Processing on {}", name());

	std::lock_guard<std::mutex> guard(_mutex);
	for (;;)
	{
		if (wasFailure())
		{
			ConnectionManager::remove(this);

			throw std::runtime_error("Error on TcpAcceptor");
		}

		sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		memset(&cliaddr, 0, clilen);

		int sock = accept(fd(), (sockaddr *)&cliaddr, &clilen);
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
				Log().debug("No more accept on {}", name());
				break;
			}

			// Ошибка установления соединения
			Log().debug("Error '{}' at accept on {}", strerror(errno), name());
			return false;
		}

		// Перевод сокета в неблокирующий режим
		int val = fcntl(sock, F_GETFL, 0);
		fcntl(sock, F_SETFL, val | O_NONBLOCK);

		// Включаем keep-alive на сокете
		val = 1; setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,  &val, sizeof(val));

		// Время до начала отправки keep-alive пакетов
		val = 5; setsockopt(sock, SOL_TCP,    TCP_KEEPIDLE,  &val, sizeof(val));

		// Количество keep-alive пакетов
		val = 1; setsockopt(sock, SOL_TCP,    TCP_KEEPCNT,   &val, sizeof(val));

		// Время между отправками
		val = 1; setsockopt(sock, SOL_TCP,    TCP_KEEPINTVL, &val, sizeof(val));

		try
		{
			TcpConnection *newConnection = new TcpConnection(sock, cliaddr);

			ConnectionManager::add(newConnection);
		}
		catch (std::exception exception)
		{
			shutdown(sock, SHUT_RDWR);
			close(sock);
		}
	}

	ConnectionManager::watch(this);

	return true;
}
