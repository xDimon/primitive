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

// TcpConnection.cpp


#include "TcpConnection.hpp"
#include "ConnectionManager.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <sys/ioctl.h>
#include "../transport/ServerTransport.hpp"
#include "../thread/ThreadPool.hpp"

TcpConnection::TcpConnection(const std::shared_ptr<Transport>& transport, int sock, const sockaddr_in &sockaddr, bool outgoing)
: Connection(transport)
, _outgoing(outgoing)
, _noRead(false)
, _noWrite(false)
{
	_sock = sock;

	_closed = _sock < 0;

	memcpy(&_sockaddr, &sockaddr, sizeof(_sockaddr));

	char ip[64];
	inet_ntop(AF_INET, &_sockaddr.sin_addr, ip, sizeof(ip));

	_name = "TcpConnection[" + std::to_string(_sock) + "][" + ip + ":" + std::to_string(htons(_sockaddr.sin_port)) + "]";

	_log.debug("%s created", name().c_str());
}

TcpConnection::~TcpConnection()
{
	shutdown(_sock, SHUT_RD);
	_log.debug("%s destroyed", name().c_str());
}

void TcpConnection::watch(epoll_event &ev)
{
	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	if (_closed)
	{
		return;
	}

	ev.events |= EPOLLERR;

	if (!_noRead)
	{
		ev.events |= EPOLLIN | EPOLLRDNORM;
	}

	ev.events |= EPOLLRDHUP;
	ev.events |= EPOLLHUP;

	if (!_error)
	{
		if (hasDataForSend())
		{
			ev.events |= EPOLLOUT | EPOLLWRNORM;
		}
	}
}

bool TcpConnection::processing()
{
	_log.debug("Begin processing on %s", name().c_str());

	do
	{
		if (timeIsOut())
		{
			_timeout = true;
			break;
		}

		if (wasFailure())
		{
			_error = true;
			break;
		}

		if (isReadyForWrite() && hasDataForSend())
		{
			writeToSocket();
		}

		if (isReadyForRead())
		{
			readFromSocket();
		}

		if (hasDataForSend())
		{
			writeToSocket();
		}

		ConnectionManager::rotateEvents(this->ptr());
	}
	while (isReadyForRead() || (isReadyForWrite() && hasDataForSend()) || wasFailure() || timeIsOut());

	if (_timeout)
	{
		_closed = true;
	}

	if (_error)
	{
		_closed = true;
	}

	if (_noRead)
	{
		if (!hasDataForSend())
		{
			_noWrite = true;

			shutdown(_sock, SHUT_RDWR);
			setTtl(std::chrono::milliseconds(50));
		}
	}

	if (_closed)
	{
		ConnectionManager::remove(ptr());

		_log.debug("End processing on %s: Closed", name().c_str());
		return true;
	}

	ConnectionManager::watch(ptr());

	if (_noRead && _noWrite)
	{
		_closed = true;
	}

	_log.debug("End processing on %s: Ready", name().c_str());
	return true;
}

bool TcpConnection::write(const void* data, size_t length)
{
	return WriterConnection::write(data, length);
}

bool TcpConnection::writeToSocket()
{
	_log.trace("Write into socket on %s", name().c_str());

	// Отправляем данные
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_outBuff.mutex());

		// Нечего отправлять
		if (!hasDataForSend())
		{
			break;
		}

		ssize_t n = ::write(_sock, _outBuff.dataPtr(), _outBuff.dataLen());
		if (n == -1)
		{
			// Повторяем вызов прерваный сигналом
			if (errno == EINTR)
			{
				continue;
			}

			// Нет возможности отправить сейчас
			if (errno == EAGAIN)
			{
				return true;
			}

			// Ошибка записи
			_log.debug("Fail writing data (error: '%s')", strerror(errno));

			_error = true;
			return false;
		}

		_outBuff.skip(static_cast<size_t>(n));
	}

	return true;
}

bool TcpConnection::readFromSocket()
{
	_log.trace("Read from socket on %s", name().c_str());

	// Пытаемся полностью заполнить буфер
	for (;;)
	{
		size_t bytes_available = 0;
		ioctl(_sock, FIONREAD, &bytes_available);

		// Нет данных на сокете
		if (bytes_available == 0)
		{
			// И больше не будет
			if (isHalfHup() || isHup())
			{
				_noRead = true;
			}
			break;
		}

		std::lock_guard<std::recursive_mutex> guard(_inBuff.mutex());

		_inBuff.prepare(bytes_available);

		ssize_t n = ::read(_sock, _inBuff.spacePtr(), _inBuff.spaceLen());
		if (n == -1)
		{
			// Повторяем вызов прерваный сигналом
			if (errno == EINTR)
			{
				continue;
			}

			// Нет готовых данных - продолжаем ждать
			if (errno == EAGAIN)
			{
				_log.debug("No more read on %s", name().c_str());
				break;
			}

			// Ошибка чтения
			_log.debug("Error '%s' while read on %s", strerror(errno), name().c_str());

			_error = true;
			return false;
		}
		if (n == 0)
		{
			// Клиент отключился
			_log.debug("Client disconnected on %s", name().c_str());

			_noRead = true;
			return false;
		}

		_inBuff.forward(static_cast<size_t>(n));

		_log.debug("Read %d bytes (summary %d) on %s", n, _inBuff.dataLen(), name().c_str());
	}

	if (_inBuff.dataLen() > 0)
	{
		auto transport = _transport.lock();
		if (transport)
		{
			transport->processing(ptr());
		}
	}

	return true;
}

void TcpConnection::close()
{
	_noRead = true;
	_inBuff.skip(_inBuff.dataLen());

	shutdown(_sock, SHUT_RD);
}

void TcpConnection::addCompleteHandler(std::function<void(TcpConnection&, const std::shared_ptr<Context>&)> handler)
{
	_completeHandler = std::move(handler);
}

void TcpConnection::addErrorHandler(std::function<void(TcpConnection&)> handler)
{
	_errorHandler = std::move(handler);
}
