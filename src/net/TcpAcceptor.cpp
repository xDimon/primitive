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


#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sstream>
#include "../log/Log.hpp"
#include "TcpAcceptor.hpp"
#include "ConnectionManager.hpp"
#include "TcpConnection.hpp"
#include <unistd.h>

TcpAcceptor::TcpAcceptor(Transport::Ptr& transport, std::string host, std::uint16_t port)
: Log("TcpAcceptor")
, Connection(transport)
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
			log().debug("Can't convert host to binary IPv4 address (error '{}'). I'll use universal address.", strerror(errno));

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
		::close(_sock);
		throw std::runtime_error(std::string("Can't bind socket: ") + strerror(errno));
	}

	// Преобразуем сокет в пассивный (слушающий) и устанавливаем длину очереди соединений
	if (listen(_sock, 16) != 0)
	{
		::close(_sock);
		throw std::runtime_error(std::string("Can't listen port: ") + strerror(errno));
	}

	// Включем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	log().debug("Create {}", name());
}

TcpAcceptor::~TcpAcceptor()
{
	log().debug("Destroy {}", name());
}

const std::string& TcpAcceptor::name()
{
	if (_name.empty())
	{
		std::ostringstream ss;
		ss << "TcpAcceptor [" << _sock << "] [" << _host << ":" << _port << "]";
		_name = ss.str();
	}
	return _name;
}

void TcpAcceptor::watch(epoll_event &ev)
{
	ev.data.ptr = new std::shared_ptr<Connection>(shared_from_this());
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	ev.events |= EPOLLERR;
	ev.events |= EPOLLIN;
}

bool TcpAcceptor::processing()
{
	log().debug("Processing on {}", name());

	std::lock_guard<std::mutex> guard(_mutex);
	for (;;)
	{
		if (wasFailure())
		{
			_closed = true;

			ConnectionManager::remove(this->ptr());

			throw std::runtime_error("Error on TcpAcceptor");
		}

		sockaddr_in cliaddr;
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
				log().debug("No more accept on {}", name());
				break;
			}

			// Ошибка установления соединения
			log().debug("Error '{}' at accept on {}", strerror(errno), name());
			return false;
		}

		// Перевод сокета в неблокирующий режим
		int val = fcntl(sock, F_GETFL, 0);
		fcntl(sock, F_SETFL, val | O_NONBLOCK);

		try
		{
			createConnection(sock, cliaddr);
		}
		catch (std::exception exception)
		{
			shutdown(sock, SHUT_RDWR);
			::close(sock);
		}
	}

	ConnectionManager::watch(this->ptr());

	return true;
}

void TcpAcceptor::createConnection(int sock, const sockaddr_in &cliaddr)
{
	Transport::Ptr transport = _transport.lock();

	auto connection = std::shared_ptr<Connection>(new TcpConnection(transport, sock, cliaddr));

	ConnectionManager::add(connection->ptr());
}
