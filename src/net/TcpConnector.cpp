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
// File created on: 2017.06.05

// TcpConnector.cpp


#include "TcpConnector.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include "ConnectionManager.hpp"
#include "../utils/Daemon.hpp"
#include "../thread/Thread.hpp"
#include "HostnameResolver.hpp"

TcpConnector::TcpConnector(const std::shared_ptr<ClientTransport>& transport, const std::string& hostname, std::uint16_t port)
: Connector(transport)
, _host(hostname)
, _port(port)
{
	_name = "TcpConnector[" + _host + ":" + std::to_string(port) + "]";

	const char* status;
	std::tie(status, _addresses) = HostnameResolver::resolve(_host, _port);
	if (status != nullptr)
	{
		throw std::runtime_error("Can't get address for '" + _host + "': " + status);
	}

	_addressesIterator = _addresses.begin();

	while (_addressesIterator != _addresses.end())
	{
		_address = reinterpret_cast<const sockaddr_storage&>(*_addressesIterator++);

		_sock = getSocket(_address.ss_family);

		// Подключаемся
		again:
		if (connect(_sock, reinterpret_cast<sockaddr*>(&_address), sizeof(_address)) == 0)
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
			Thread::self()->yield([]{});
			goto again;
		}
	}

	throw std::runtime_error("Can't connect to '" + _host + "' ← " + strerror(errno));

	end:

	_name += "(" + std::to_string(_sock) + ")";

	_log.debug("%s created", name().c_str());
}

TcpConnector::~TcpConnector()
{
	_log.debug("%s destroyed", name().c_str());
}

int TcpConnector::getSocket(int family)
{
	if (_sock != -1)
	{
		::close(_sock);
	}

	// Создаем сокет
	_sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == -1)
	{
		throw std::runtime_error("Can't create socket");
	}
	_closed = false;

	const int val = 1;
	setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	// Включаем неблокирующий режим
	int rrc = fcntl(_sock, F_GETFL, 0);
	fcntl(_sock, F_SETFL, rrc | O_NONBLOCK);

	return _sock;
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
		ConnectionManager::remove(ptr());
		return false;
	}

	int result;
	socklen_t result_len = sizeof(result);

	if (!isReadyForWrite() || isHup() || wasFailure())
	{
		goto nextAddress;
	}

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

	nextAddress:
	while (_addressesIterator != _addresses.end())
	{
		_address = *_addressesIterator++;

		ConnectionManager::remove(ptr());

		_sock = getSocket(_address.ss_family);

		ConnectionManager::add(ptr());

		again:
		// Подключаемся
		if (connect(_sock, reinterpret_cast<sockaddr*>(&_address), sizeof(_address)) == 0)
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
			Thread::self()->yield([]{});
			goto again;
		}
	}

	_log.debug("End processing on %s: Fail '%s'", name().c_str(), strerror(result));

	ConnectionManager::remove(ptr());

	shutdown(_sock, SHUT_RDWR);
	onError();

	_closed = true;
	_sock = -1;

	return false;
}

std::shared_ptr<TcpConnection> TcpConnector::createConnection(const std::shared_ptr<Transport>& transport)
{
	return std::make_shared<TcpConnection>(transport, _sock, _address, true);
}

void TcpConnector::addConnectedHandler(std::function<void(const std::shared_ptr<TcpConnection>&)> handler)
{
	_connectHandler = std::move(handler);
}

void TcpConnector::addErrorHandler(std::function<void()> handler)
{
	_errorHandler = std::move(handler);
}
