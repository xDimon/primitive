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

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ConnectionManager.hpp"

SslConnection::SslConnection(Transport::Ptr& transport, int sock, const sockaddr_in& sockaddr, std::shared_ptr<SSL_CTX> sslContext)
: Log("SslConnection")
, TcpConnection(transport, sock, sockaddr)
, _sslEstablished(false)
{
	_sslContext = sslContext;

	_sslConnect = SSL_new(_sslContext.get());
	SSL_set_fd(_sslConnect, _sock);

	log().debug("Create {}", name());
}

SslConnection::~SslConnection()
{
	log().debug("Destroy {}", name());

	SSL_shutdown(_sslConnect);
	SSL_free(_sslConnect);

	shutdown(_sock, SHUT_RD);
}

const std::string& SslConnection::name()
{
	if (_name.empty())
	{
		std::ostringstream ss;
		ss << "SslConnection [" << _sock << "] [" << inet_ntoa(_sockaddr.sin_addr) << ":" << htons(_sockaddr.sin_port) << "]";
		_name = ss.str();
	}
	return _name;
}

bool SslConnection::processing()
{
	log().debug("Processing on {}", name());

	do
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (wasFailure())
		{
			_closed = true;
			break;
		}

		if (!_sslEstablished)
		{
			int n = SSL_accept(_sslConnect);
			if(n <= 0)
			{
				auto e = SSL_get_error(_sslConnect, n);
				// Повторяем вызов прерваный сигналом
				if (e == SSL_ERROR_SYSCALL)
				{
					if (isHalfHup() || isHup())
					{
						log().trace("Can't complete SSL-handshake: already closed {}", name());
						_noRead = true;
						_noWrite = true;
						break;
					}

					if (errno)
					{
						log().debug("SSL_ERROR_SYSCALL while SSL-handshake on {}: {}", name(), strerror(errno));
						break;
					}

					log().debug("SSL_ERROR_SYSCALL with errno=0 while SSL-handshake on {}", name());
					break;
				}
				if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE)
				{
					log().trace("SSL-handshake incomplete yet on {}", name());
					break;
				}

				char err[1<<12];
				ERR_error_string_n(ERR_get_error(), err, sizeof(err));

				log().trace("Fail SSL-handshake on {}: {}", name(), err);
				_error = true;
				break;
			}

			_sslEstablished = true;

			log().trace("Success SSL-handshake on {}", name());
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
	}

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

bool SslConnection::writeToSocket()
{
	log().trace("Write into socket on {}", name());

	// Отправляем данные
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_outBuff.mutex());

		// Нечего отправлять
		if (_outBuff.dataLen() == 0)
		{
			break;
		}

		int n = SSL_write(_sslConnect, _outBuff.dataPtr(), _outBuff.dataLen());
		if (n > 0)
		{
			_outBuff.skip(n);
			log().debug("Write {} bytes on {}", n, name());
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
			log().debug("No more write on {}", name());
			break;
		}
		else if (e == SSL_ERROR_WANT_READ)
		{
			log().debug("No more write without read before on {}", name());
			break;
		}
		// Повторяем вызов прерваный сигналом
		else if (e == SSL_ERROR_SYSCALL)
		{
			if (isHup())
			{
				log().debug("SSL_ERROR_SYSCALL Can't more write {}", name());
				_noWrite = true;
				break;
			}

			if (errno)
			{
				log().debug("SSL_ERROR_SYSCALL while SSL-write on {}: {}", name(), strerror(errno));
				break;
			}

			log().debug("SSL_ERROR_SYSCALL with errno=0 while SSL-write on {}", name());
			break;
		}
		else if (e == SSL_ERROR_SSL)
		{
			log().trace("SSL_ERROR_SSL while SSL-write on {}", name());
			_error = true;
			return false;
		}
		else
		{
			log().trace("SSL_ERROR_? while SSL-write on {}", name());
			_error = true;
			return false;
		}
	}

	return true;
}

bool SslConnection::readFromSocket()
{
	log().trace("Read from socket on {}", name());

	// Пытаемся полностью заполнить буфер
	for (;;)
	{
		std::lock_guard<std::recursive_mutex> guard(_inBuff.mutex());

		_inBuff.prepare(1<<12);

		int n = SSL_read(_sslConnect, _inBuff.spacePtr(), _inBuff.spaceLen());
		if (n > 0)
		{
			_inBuff.forward(n);
			log().trace("Read {} bytes on {}", n, name());
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
			log().trace("No more read on {}", name());
			break;
		}
		// Нет готовых данных - продолжаем ждать
		else if (e == SSL_ERROR_ZERO_RETURN)
		{
			// Клиент отключился
			log().trace("Client disconnected on {}", name());
			_noRead = true;
			break;
		}
		else if (e == SSL_ERROR_WANT_WRITE)
		{
			log().trace("No more read without writing before on {}", name());
			break;
		}
		else if (e == SSL_ERROR_SYSCALL)
		{
			if (isHalfHup() || isHup())
			{
				log().trace("Connection already closed {}", name());
				_noRead = true;
				break;
			}

			if (errno)
			{
				log().debug("SSL_ERROR_SYSCALL while SSL-read on {}: {}", name(), strerror(errno));
				break;
			}

			log().trace("SSL_ERROR_SYSCALL with errno=0 while SSL-read on {}", name());
			break;
		}
		else if (e == SSL_ERROR_SSL)
		{
			log().debug("SSL_ERROR_SSL while SSL-read on {}", name());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			_error = true;
			return false;
		}
		else
		{
			log().debug("SSL_ERROR_? while SSL-read on {}", name());
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
