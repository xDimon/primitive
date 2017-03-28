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
#include <sys/ioctl.h>
#include <cstring>

TcpConnection::TcpConnection(Transport::Ptr& transport, int sock, const sockaddr_in &sockaddr)
: Log("TcpConnection")
, Connection(transport)
, ReaderConnection()
, WriterConnection()
, _noRead(false)
, _noWrite(false)
, _error(false)
, _closed(false)
{
	_sock = sock;

	memcpy(&_sockaddr, &sockaddr, sizeof(_sockaddr));

	log().debug("Create {}", name());
}

TcpConnection::~TcpConnection()
{
	log().debug("Destroy {}", name());

	shutdown(_sock, SHUT_RD);
}

const std::string& TcpConnection::name()
{
	if (_name.empty())
	{
		std::ostringstream ss;
		ss << "TcpConnection [" << _sock << "] [" << inet_ntoa(_sockaddr.sin_addr) << ":" << htons(_sockaddr.sin_port) << "]";
		_name = ss.str();
	}
	return _name;
}

void TcpConnection::watch(epoll_event &ev)
{
	ev.data.ptr = new std::shared_ptr<Connection>(shared_from_this());
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
		if (_outBuff.dataLen() > 0)
		{
			ev.events |= EPOLLOUT | EPOLLWRNORM;
		}
	}
}

bool TcpConnection::processing()
{
	log().debug("Processing on {}", name());

	do
	{
		if (wasFailure())
		{
			_closed = true;
			ConnectionManager::remove(this->ptr());
			return true;
		}

		if (isReadyForWrite())
		{
			writeToSocket();
		}

		if (isReadyForRead())
		{
			readFromSocket();
		}

		if (_outBuff.dataLen() > 0)
		{
			writeToSocket();
		}

		ConnectionManager::rotateEvents(this->ptr());
	}
	while (isReadyForRead() || (_outBuff.dataLen() > 0 && isReadyForWrite()));

	if (_noRead)
	{
		if (!_outBuff.dataLen())
		{
			shutdown(_sock, SHUT_WR);
			_noWrite = true;
		}
	}

	if (_noRead && _noWrite)
	{
		_closed = true;
	}

	if (_closed)
	{
		ConnectionManager::remove(this->ptr());

		return true;
	}

	ConnectionManager::watch(this->ptr());

	return true;
}

bool TcpConnection::writeToSocket()
{
	log().debug("Write into socket on {}", name());

	// Отправляем данные
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_outBuff.mutex());

		// Нечего отправлять
		if (_outBuff.dataLen() == 0)
		{
			break;
		}

		int n = ::write(_sock, _outBuff.dataPtr(), _outBuff.dataLen());
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
			log().debug("Fail writing data (error: '')", strerror(errno));

			_error = true;

			return false;
		}

		_outBuff.skip(n);
	}

	return true;
}

bool TcpConnection::readFromSocket()
{
	log().debug("Read from into socket on {}", name());

	// Пытаемся полностью заполнить буфер
	for (;;)
	{
		size_t bytes_available = 0;
		ioctl(_sock, FIONREAD, &bytes_available);

		// Нет данных на сокете
		if (!bytes_available)
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

		int n = ::read(_sock, _inBuff.spacePtr(), _inBuff.spaceLen());
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
				log().debug("No more read on {}", name());
				break;
			}

			// Ошибка чтения
			log().debug("Error '{}' while read on {}", strerror(errno), name());

			return false;
		}
		if (n == 0)
		{
			// Клиент отключился
			log().debug("Client disconnected on {}", name());

			return false;
		}

		_inBuff.forward(n);

//		char buff[1<<12];
//		size_t len = std::min(_inBuff.dataLen(), sizeof(buff)-1);
//		_inBuff.show(buff, len);
//		buff[len] = 0;
//		std::string b(buff);

		log().debug("Read {} bytes (summary {}) on {}", n, _inBuff.dataLen(), name());
	}

	auto transport = _transport.lock();
	if (transport)
	{
		transport->processing(ptr());
	}

	return true;
}

void TcpConnection::close()
{
	shutdown(_sock, SHUT_RD);
	_noRead = true;
}
