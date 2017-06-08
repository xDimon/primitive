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
// File created on: 2017.06.05

// TcpConnector.cpp


#include "TcpConnector.hpp"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <sys/param.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "ConnectionManager.hpp"
#include "TcpConnection.hpp"
#include "../utils/ShutdownManager.hpp"
#include "../thread/ThreadPool.hpp"

const int TcpConnector::timeout;

TcpConnector::~TcpConnector()
{
	_log.debug("TcpConnector '%s' destroyed", name().c_str());
}

TcpConnector::TcpConnector(std::shared_ptr<ClientTransport>& transport, std::string hostname, std::uint16_t port)
: Connector(transport)
, _host(std::move(hostname))
, _port(port)
, _buff( reinterpret_cast<char*>(malloc(1024)))
, _buffSize(1024)
, _hostptr(nullptr)
, _addrIterator(nullptr)
{
	// Создаем сокет
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == -1)
	{
		throw std::runtime_error("Can't create socket");
	}
	_closed = false;

	std::ostringstream ss;
	ss << "[" << _sock << "][" << _host << ":" << port << "]";
	_name = std::move(ss.str());

	const int val = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	// Включем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	int herr = 0;
	int hres = 0;

	// Список адресов по хосту
	while ((hres = gethostbyname_r(_host.c_str(), &_hostbuf, _buff, _buffSize, &_hostptr, &herr)) == ERANGE)
	{
		// realloc
		_buffSize <<= 1;
		char* tmp = reinterpret_cast<char*>(realloc(_buff, _buffSize));
		if (!tmp)
		{
			// OOM?
			_buffSize >>= 1;
			_buffSize += 64;
			tmp = reinterpret_cast<char*>(realloc(_buff, _buffSize));
			if (!tmp)
			{ // OOM!
				throw std::bad_alloc();
			}
		}

		_buff = tmp;
	}

	if (!_hostptr)
	{
		// error translation.
		switch (herr)
		{
			case HOST_NOT_FOUND:
				throw std::runtime_error(std::string("Host not found ") + _host);
			case NO_ADDRESS:
				throw std::runtime_error(std::string() + "The requested name ("  + _host + ") does not have an IP address");
			case NO_RECOVERY:
				throw std::runtime_error(std::string() + "A non-recoverable name server error occurred while resolving '"  + _host + "'");
			case TRY_AGAIN:
				throw std::runtime_error(std::string() + "A temporary error occurred on an authoritative name server while resolving '"  + _host + "'");
			default:
				throw std::runtime_error(std::string() + "Unknown error code from gethostbyname_r for '" + _host + "'");
		}
	}

	_addrIterator = _hostptr->h_addr_list;

	for ( ; *_addrIterator; ++_addrIterator)
	{
		// Инициализируем структуру нулями
		memset(&_sockaddr, 0, sizeof(_sockaddr));

		// Задаем семейство сокетов (IPv4)
		_sockaddr.sin_family = AF_INET;

		// Задаем хост
		memcpy(&_sockaddr.sin_addr.s_addr, *_addrIterator, sizeof(_sockaddr.sin_addr.s_addr));

		// Задаем порт
		_sockaddr.sin_port = htons(_port);

		// Подключаемся
		again:
		if (!connect(_sock, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof(_sockaddr)))
		{
			throw std::runtime_error(std::string("Too fast connect to ") + _host);
//
//			// Подключились сразу?!
//			createConnection(_sock, _sockaddr);
//			_sock = -1;
//			_closed = true;
//			goto end;
		}

		// Вызов прерван сигналом - повторяем
		if (errno == EINTR)
		{
			goto again;
		}

		// Установление соединения в процессе
		if (errno == EINPROGRESS)
		{
			goto end;
		}
	}

	throw std::runtime_error(std::string() + "Can't connect to '" + _host + "' ← " + strerror(errno));

	end:

	_log.debug("TcpConnector '%s' created", name().c_str());
}

void TcpConnector::watch(epoll_event& ev)
{
	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	ev.events |= EPOLLERR;
	ev.events |= EPOLLOUT;
}

bool TcpConnector::processing()
{
	_log.debug("Processing on %s", name().c_str());

	std::lock_guard<std::mutex> guard(_mutex);
//	for (;;)
//	{
//		if (ShutdownManager::shutingdown())
//		{
//			_log.debug("Interrupt processing on %s (shutdown)", name().c_str());
//			ConnectionManager::remove(this->ptr());
//			return false;
//		}

		int result;
		socklen_t result_len = sizeof(result);

		if (!getsockopt(_sock, SOL_SOCKET, SO_ERROR, &result, &result_len))
		{
			if (!result)
			{
				connected:

				_closed = true;

				_log.debug("End processing on %s (was connected)", name().c_str());

				ConnectionManager::remove(ptr());

				try
				{
					createConnection(_sock, _sockaddr);
					_sock = -1;
					return true;
				}
				catch (std::exception exception)
				{
					onError();
					shutdown(_sock, SHUT_RDWR);
					::close(_sock);
					return false;
				}
			}
		}

		for ( ; *_addrIterator; ++_addrIterator)
		{
			// Инициализируем структуру нулями
			memset(&_sockaddr, 0, sizeof(_sockaddr));

			// Задаем семейство сокетов (IPv4)
			_sockaddr.sin_family = AF_INET;

			// Задаем хост
			memcpy(&_sockaddr.sin_addr.s_addr, *_addrIterator, sizeof(_sockaddr.sin_addr.s_addr));

			// Задаем порт
			_sockaddr.sin_port = htons(_port);

			again:
			// Подключаемся
			if (!connect(_sock, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof(_sockaddr)))
			{
				// Подключились сразу?!
				goto connected;
			}

			// Вызов прерван сигналом - повторяем
			if (errno == EINTR)
			{
				goto again;
			}

			// Установление соединения в процессе
			if (errno == EINPROGRESS)
			{
				return true;
			}
		}

		_log.debug("End processing on %s (was failure)", name().c_str());

		ConnectionManager::remove(ptr());

		onError();
		shutdown(_sock, SHUT_RDWR);
		::close(_sock);
		return false;
//	}
}

void TcpConnector::createConnection(int sock, const sockaddr_in& cliaddr)
{
	std::shared_ptr<Transport> transport = _transport.lock();
	if (!transport)
	{
		return;
	}

	auto connection = std::make_shared<TcpConnection>(transport, sock, cliaddr, true);

	onConnect(connection);

	ThreadPool::enqueue(
		[wp = std::weak_ptr<Connection>(connection->ptr())]() {
			auto connection = std::dynamic_pointer_cast<TcpConnection>(wp.lock());
			if (!connection)
			{
				return;
			}
			if (std::chrono::steady_clock::now() > connection->aTime() + std::chrono::seconds(timeout))
			{
				Log("Timeout").debug("Connection '%s' closed by timeout", connection->name().c_str());
				connection->close();
			}
		}
		, std::chrono::seconds(timeout)
	);

	ConnectionManager::remove(ptr());
	ConnectionManager::add(connection);
}

void TcpConnector::addConnectedHandler(std::function<void(std::shared_ptr<TcpConnection>)> handler)
{
	_connectHandler = std::move(handler);
}

void TcpConnector::addErrorHandler(std::function<void()> handler)
{
	_errorHandler = std::move(handler);
}
