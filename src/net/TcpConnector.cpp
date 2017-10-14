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
#include <fcntl.h>
#include <cstring>

#include "ConnectionManager.hpp"
#include "../utils/Daemon.hpp"
#include "../thread/Coroutine.hpp"

TcpConnector::TcpConnector(const std::shared_ptr<ClientTransport>& transport, const std::string& hostname, std::uint16_t port)
: Connector(transport)
, _host(hostname)
, _port(port)
, _buff(reinterpret_cast<char*>(malloc(1024)))
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

	_name = "TcpConnector[" + std::to_string(_sock) + "][" + _host + ":" + std::to_string(port) + "]";

	const int val = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	// Включем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	int herr = 0;

	// Список адресов по хосту
	while (gethostbyname_r(_host.c_str(), &_hostbuf, _buff, _buffSize, &_hostptr, &herr) == ERANGE)
	{
		// realloc
		_buffSize <<= 1;
		auto tmp = reinterpret_cast<char*>(realloc(_buff, _buffSize));
		if (tmp == nullptr)
		{
			// OOM?
			_buffSize >>= 1;
			_buffSize += 64;
			tmp = reinterpret_cast<char*>(realloc(_buff, _buffSize));
			if (tmp == nullptr)
			{ // OOM!
				throw std::bad_alloc();
			}
		}

		_buff = tmp;
	}

	if (_hostptr == nullptr)
	{
		// error translation.
		switch (herr)
		{
			case HOST_NOT_FOUND:
				throw std::runtime_error("Host not found " + _host);
			case NO_ADDRESS:
				throw std::runtime_error("The requested name ("  + _host + ") does not have an IP address");
			case NO_RECOVERY:
				throw std::runtime_error("A non-recoverable name server error occurred while resolving '"  + _host + "'");
			case TRY_AGAIN:
				throw std::runtime_error("A temporary error occurred on an authoritative name server while resolving '"  + _host + "'");
			default:
				throw std::runtime_error("Unknown error code from gethostbyname_r for '" + _host + "'");
		}
	}

	_addrIterator = _hostptr->h_addr_list;

	errno = 0;

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
		if (connect(_sock, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof(_sockaddr)) == 0)
		{
			throw std::runtime_error(std::string("Too fast connect to ") + _host);
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

		// Нет доступных пар адрес-порт для исходящего соединения
		if (errno == EADDRNOTAVAIL)
		{
			Coroutine(std::make_shared<Task>(std::make_shared<Task::Func>([](){})));
			goto again;
		}
	}

	throw std::runtime_error("Can't connect to '" + _host + "' ← " + strerror(errno));

	end:

	_log.debug("%s created", name().c_str());
}

TcpConnector::~TcpConnector()
{
	free(_buff);
	_log.debug("%s destroyed", name().c_str());
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
	_log.debug("Begin processing on %s", name().c_str());

	std::lock_guard<std::mutex> guard(_mutex);

	if (Daemon::shutingdown())
	{
		_log.debug("Interrupt processing on %s (shutingdown)", name().c_str());
		ConnectionManager::remove(this->ptr());
		return false;
	}

	int result;
	socklen_t result_len = sizeof(result);

	if (getsockopt(_sock, SOL_SOCKET, SO_ERROR, &result, &result_len) == 0)
	{
		if (result == 0)
		{
			connected:

			_log.debug("End processing on %s: Success", name().c_str());

			try
			{
				std::shared_ptr<Transport> transport = _transport.lock();
				if (!transport)
				{
					throw std::runtime_error("Lost transport");
				}

				auto newConnection = createConnection(transport);

				ConnectionManager::remove(ptr());

				_sock = -1;
				_closed = true;

				newConnection->setTtl(std::chrono::seconds(60));

				ConnectionManager::add(newConnection);

				onConnect(newConnection);

				return true;
			}
			catch (const std::exception& exception)
			{
				onError();
				shutdown(_sock, SHUT_RDWR);
				::close(_sock);
				return false;
			}
		}
	}

	for ( ; *_addrIterator != nullptr; ++_addrIterator)
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
		if (connect(_sock, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof(_sockaddr)) == 0)
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
			_log.debug("End processing on %s: In progress", name().c_str());
			return true;
		}

		// Нет доступных пар адрес-порт для исходящего соединения
		if (errno == EADDRNOTAVAIL)
		{
			Coroutine(std::make_shared<Task>(std::make_shared<Task::Func>([](){})));
			goto again;
		}
	}

	_log.debug("End processing on %s: Fail '%s'", name().c_str(), strerror(result));

	ConnectionManager::remove(ptr());

	shutdown(_sock, SHUT_RDWR);
	onError();
	return false;
}

std::shared_ptr<TcpConnection> TcpConnector::createConnection(const std::shared_ptr<Transport>& transport)
{
	return std::make_shared<TcpConnection>(transport, _sock, _sockaddr, true);
}

void TcpConnector::addConnectedHandler(std::function<void(const std::shared_ptr<TcpConnection>&)> handler)
{
	_connectHandler = std::move(handler);
}

void TcpConnector::addErrorHandler(std::function<void()> handler)
{
	_errorHandler = std::move(handler);
}
