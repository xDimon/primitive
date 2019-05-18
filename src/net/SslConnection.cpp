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
// File created on: 2017.02.25

// SslConnection.cpp


#include "SslConnection.hpp"
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ConnectionManager.hpp"
#include "../transport/ServerTransport.hpp"

SslConnection::SslConnection(const std::shared_ptr<Transport>& transport, int sock, const sockaddr_in& sockaddr, const std::shared_ptr<SSL_CTX>& sslContext, bool outgoing)
: TcpConnection(transport, sock, sockaddr, outgoing)
, _sslContext(sslContext)
, _sslEstablished(false)
, _sslWantRead(!outgoing)
, _sslWantWrite(outgoing)
{
	_name = "SslConnection" + _name.substr(13);

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
		_log.trace("WATCH: Established and can read");
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
		else if (hasDataForSend())
		{
			_log.trace("WATCH: Established and has data for write");
			ev.events |= EPOLLOUT | EPOLLWRNORM;
		}
	}
}

bool SslConnection::processing()
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

		if (!_sslEstablished)
		{
			if (!sslHandshake())
			{
				break;
			}
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

		if (isHup())
		{
			_noRead = true;
			_noWrite = true;
			setTtl(std::chrono::milliseconds(50));
			break;
		}
		else if (isHalfHup())
		{
			_noRead = true;
			setTtl(std::chrono::milliseconds(50));
			break;
		}

		ConnectionManager::rotateEvents(this->ptr());
	}
	while (isReadyForRead() || (isReadyForWrite() && hasDataForSend()) || wasFailure() || timeIsOut());

	if (_timeout)
	{
		_closed = true;
		onError();
	}

	if (_error)
	{
		_closed = true;
		onError();
	}

	if (_noRead)
	{
		if (!hasDataForSend())
		{
			shutdown(_sock, SHUT_WR);
			setTtl(std::chrono::milliseconds(50));
			_noWrite = true;
		}
		onComplete();
	}

	if (_closed)
	{
		ConnectionManager::remove(ptr());

		_log.debug("End processing on %s: Close", name().c_str());
		return true;
	}

	ConnectionManager::watch(this->ptr());

	if (_noRead && _noWrite)
	{
		_closed = true;
	}

	_log.debug("End processing on %s: Ready", name().c_str());
	return true;
}

bool SslConnection::sslHandshake()
{
	again:

	int n = _outgoing ? SSL_connect(_sslConnect) : SSL_accept(_sslConnect);
	if(n <= 0)
	{
		auto e = SSL_get_error(_sslConnect, n);
		// Повторяем вызов прерваный сигналом
		if (e == SSL_ERROR_SYSCALL)
		{
			if (isHup() || isHalfHup())
			{
				_log.trace("Can't complete SSH handshake: already closed %s", name().c_str());
				shutdown(_sock, SHUT_RD);
				setTtl(std::chrono::milliseconds(50));
				_noRead = true;
				if (isHup())
				{
					_noWrite = true;
				}
				return false;
			}

			if (errno)
			{
				_error = true;
				_log.debug("SSL_ERROR_SYSCALL while SSH handshake on %s: %s", name().c_str(), strerror(errno));
				return false;
			}

			_log.debug("SSL_ERROR_SYSCALL with errno=0 while SSH handshake on %s", name().c_str());
			goto again;
		}
		if (e == SSL_ERROR_WANT_READ)
		{
			_sslWantRead = true;
			_sslWantWrite = false;
			_log.trace("SSH handshake incomplete yet on %s (want read)", name().c_str());
			return false;
		}
		if (e == SSL_ERROR_WANT_WRITE)
		{
			_sslWantRead = false;
			_sslWantWrite = true;
			_log.trace("SSH handshake incomplete yet on %s (want wtite)", name().c_str());
			return false;
		}

		char err[1ull<<7];
		ERR_error_string_n(ERR_get_error(), err, sizeof(err));

		_log.trace("Fail SSH handshake on %s: %s", name().c_str(), err);

		char msg[] = "HTTP/1.1 525 SSL handshake failed\r\n\r\nSSL handshake failed\n";
		::write(_sock, msg, sizeof(msg));

		shutdown(_sock, SHUT_RD);
		setTtl(std::chrono::milliseconds(50));
		_noRead = true;
		_noWrite = true;
		return false;
	}

	_sslWantRead = false;
	_sslWantWrite = false;

	_sslEstablished = true;

	_log.trace("Success SSH handshake on %s", name().c_str());
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
		if (!hasDataForSend())
		{
			break;
		}

		int n = SSL_write(_sslConnect, _outBuff.dataPtr(), (_outBuff.dataLen() > 1ull<<12u) ? (1ull<<12u) : static_cast<int>(_outBuff.dataLen()));
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

		_inBuff.prepare(1ull<<12u);

		int n = SSL_read(_sslConnect, _inBuff.spacePtr(), (_inBuff.spaceLen() > 1ull<<12u) ? (1ull<<12u) : static_cast<int>(_inBuff.spaceLen()));
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
