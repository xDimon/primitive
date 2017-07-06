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
// File created on: 2017.02.25

// SslConnection.cpp


#include "SslConnection.hpp"

#include <sstream>
#include <openssl/err.h>
#include <unistd.h>

#include "ConnectionManager.hpp"
#include "../transport/ServerTransport.hpp"

SslConnection::SslConnection(const std::shared_ptr<Transport>& transport, int sock, const sockaddr_in& sockaddr, const std::shared_ptr<SSL_CTX>& sslContext, bool outgoing)
: TcpConnection(transport, sock, sockaddr, outgoing)
, _sslContext(sslContext)
, _sslEstablished(false)
, _sslWantRead(!outgoing)
, _sslWantWrite(outgoing)
{
	_sslConnect = SSL_new(_sslContext.get());
	SSL_set_fd(_sslConnect, _sock);
}

SslConnection::~SslConnection()
{
	SSL_shutdown(_sslConnect);
	SSL_free(_sslConnect);

	shutdown(_sock, SHUT_RD);
}

void SslConnection::watch(epoll_event &ev)
{
	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	if (_closed)
	{
		return;
	}

	ev.events |= EPOLLERR;


	if (!_sslEstablished)
	{
		if (_sslWantRead)
		{
			_log.trace("WATCH: No established and want read");
			ev.events |= EPOLLIN | EPOLLRDNORM;
		}
		else
		{
			_log.trace("WATCH: No established and no want read");
		}
	}
	else if (!_noRead)
	{
		_log.trace("WATCH: no not read");
		ev.events |= EPOLLIN | EPOLLRDNORM;
	}

	ev.events |= EPOLLRDHUP;
	ev.events |= EPOLLHUP;

	if (!_error)
	{
		if (!_sslEstablished)
		{
			if (_sslWantWrite)
			{
				_log.trace("WATCH: No established and want write");
				ev.events |= EPOLLOUT | EPOLLWRNORM;
			}
			else
			{
				_log.trace("WATCH: No established and no want write");
			}
		}
		else if (_outBuff.dataLen() > 0)
		{
			_log.trace("WATCH: has data for write");
			ev.events |= EPOLLOUT | EPOLLWRNORM;
		}
	}
}

bool SslConnection::processing()
{
	_log.debug("Processing on %s", name().c_str());

	do
	{
		if (wasFailure())
		{
			_error = true;
			break;
		}

		if (!_sslEstablished)
		{
			int n = _outgoing
					? SSL_connect(_sslConnect)
					: SSL_accept(_sslConnect);
			if(n <= 0)
			{
				_sslWantRead = false;
				_sslWantWrite = false;

				auto e = SSL_get_error(_sslConnect, n);
				// Повторяем вызов прерваный сигналом
				if (e == SSL_ERROR_SYSCALL)
				{
					if (isHalfHup() || isHup())
					{
						_log.trace("Can't complete SSH handshake: already closed %s", name().c_str());
						shutdown(_sock, SHUT_RD);
						_noRead = true;
//						_noWrite = true;
						break;
					}

					if (errno)
					{
						_log.debug("SSL_ERROR_SYSCALL while SSH handshake on %s: %s", name().c_str(), strerror(errno));
						break;
					}

					_log.debug("SSL_ERROR_SYSCALL with errno=0 while SSH handshake on %s", name().c_str());
					break;
				}
				if (e == SSL_ERROR_WANT_READ)
				{
					_sslWantRead = true;
					_log.trace("SSH handshake incomplete yet on %s (want read)", name().c_str());
					break;
				}
				if (e == SSL_ERROR_WANT_WRITE)
				{
					_sslWantRead = true;
					_sslWantWrite = true;
					_log.trace("SSH handshake incomplete yet on %s (want wtite)", name().c_str());
					break;
				}

				char err[1<<7];
				ERR_error_string_n(ERR_get_error(), err, sizeof(err));

				_log.trace("Fail SSH handshake on %s: %s", name().c_str(), err);

				char msg[] = "HTTP/1.1 525 SSL handshake failed\r\n\r\nSSL handshake failed\n";
				::write(_sock, msg, sizeof(msg));

				shutdown(_sock, SHUT_RD);
				_noRead = true;
				_noWrite = true;
				break;
			}

			_sslEstablished = true;

			_log.trace("Success SSH handshake on %s", name().c_str());
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

	if (_error)
	{
		_closed = true;
		onError();
	}

	if (_noRead)
	{
		if (!_outBuff.dataLen())
		{
			shutdown(_sock, SHUT_WR);
			_noWrite = true;
		}
		onComplete();
	}

	if (_closed)
	{
		ConnectionManager::remove(ptr());

		return true;
	}

	ConnectionManager::watch(this->ptr());

	if (_noRead && _noWrite)
	{
		_closed = true;
	}

	return true;
}

bool SslConnection::writeToSocket()
{
	_log.trace("Write into socket on %s", name().c_str());

	// Отправляем данные
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_outBuff.mutex());

		// Нечего отправлять
		if (_outBuff.dataLen() == 0)
		{
			break;
		}

		int n = SSL_write(_sslConnect, _outBuff.dataPtr(), (_outBuff.dataLen() > 1<<12) ? (1<<12) : static_cast<int>(_outBuff.dataLen()));
		if (n > 0)
		{
			_outBuff.skip(static_cast<size_t>(n));
			_log.debug("Write %d bytes on %s", n, name().c_str());
			continue;
		}

		auto e = SSL_get_error(_sslConnect, n);
		if (e == SSL_ERROR_NONE)
		{
			if (n == 0)
			{
				continue;
			}
		}
		else if (e == SSL_ERROR_WANT_WRITE)
		{
			_log.debug("No more write on %s", name().c_str());
			break;
		}
		else if (e == SSL_ERROR_WANT_READ)
		{
			_log.debug("No more write without read before on %s", name().c_str());
			break;
		}
		// Повторяем вызов прерваный сигналом
		else if (e == SSL_ERROR_SYSCALL)
		{
			if (isHup())
			{
				_log.debug("SSL_ERROR_SYSCALL Can't more write %s", name().c_str());
				_noWrite = true;
				break;
			}

			if (errno)
			{
				_log.debug("SSL_ERROR_SYSCALL while SslHelper-write on %s: %s", name().c_str(), strerror(errno));
				break;
			}

			_log.debug("SSL_ERROR_SYSCALL with errno=0 while SslHelper-write on %s", name().c_str());
			break;
		}
		else if (e == SSL_ERROR_SSL)
		{
			_log.trace("SSL_ERROR_SSL while SslHelper-write on %s", name().c_str());
			_error = true;
			return false;
		}
		else
		{
			_log.trace("SSL_ERROR_? while SslHelper-write on %s", name().c_str());
			_error = true;
			return false;
		}
	}

	return true;
}

bool SslConnection::readFromSocket()
{
	_log.trace("Read from socket on %s", name().c_str());

	// Пытаемся полностью заполнить буфер
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_inBuff.mutex());

		_inBuff.prepare(1<<12);

		int n = SSL_read(_sslConnect, _inBuff.spacePtr(), (_inBuff.spaceLen() > 1<<12) ? (1<<12) : static_cast<int>(_inBuff.spaceLen()));
		if (n > 0)
		{
			_inBuff.forward(static_cast<size_t>(n));
			_log.trace("Read %d bytes on %s", n, name().c_str());
			continue;
		}

		auto e = SSL_get_error(_sslConnect, n);
		if (e == SSL_ERROR_NONE)
		{
			if (n == 0)
			{
				continue;
			}
		}
		else if (e == SSL_ERROR_WANT_READ)
		{
			_log.trace("No more read on %s", name().c_str());
			break;
		}
		// Нет готовых данных - продолжаем ждать
		else if (e == SSL_ERROR_ZERO_RETURN)
		{
			// Клиент отключился
			_log.trace("Client disconnected on %s", name().c_str());
			_noRead = true;
			break;
		}
		else if (e == SSL_ERROR_WANT_WRITE)
		{
			_log.trace("No more read without writing before on %s", name().c_str());
			break;
		}
		else if (e == SSL_ERROR_SYSCALL)
		{
			if (isHalfHup() || isHup())
			{
				_log.trace("Connection already closed %s", name().c_str());
				_noRead = true;
				break;
			}

			if (errno)
			{
				_log.debug("SSL_ERROR_SYSCALL while SslHelper-read on %s: %s", name().c_str(), strerror(errno));
				break;
			}

			_log.trace("SSL_ERROR_SYSCALL with errno=0 while SslHelper-read on %s", name().c_str());
			break;
		}
		else if (e == SSL_ERROR_SSL)
		{
			_log.debug("SSL_ERROR_SSL while SslHelper-read on %s", name().c_str());
			_error = true;
			return false;
		}
		else
		{
			_log.debug("SSL_ERROR_? while SslHelper-read on %s", name().c_str());
			_error = true;
			return false;
		}
	}

	if (_inBuff.dataLen())
	{
		auto transport = _transport.lock();
		if (transport)
		{
			transport->processing(ptr());
		}
	}

	return true;
}
