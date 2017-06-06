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

// TcpConnection.hpp


#pragma once

#include "Connection.hpp"
#include "../utils/Buffer.hpp"
#include "ReaderConnection.hpp"
#include "WriterConnection.hpp"
#include <netinet/in.h>

class TcpConnection: public Connection, public ReaderConnection, public WriterConnection
{
public:
	static const int timeout = 900;

protected:
	bool _outgoing;

	sockaddr_in _sockaddr;

	/// Данных больше не будет
	bool _noRead;

	/// Писать больше не будем
	bool _noWrite;

	/// Ошибка соединения
	bool _error;

	/// Соединение закрыто
	bool _closed;

	virtual bool readFromSocket();

	virtual bool writeToSocket();

	std::chrono::steady_clock::time_point _accessTime;

	std::function<void(std::shared_ptr<Context>&)> _completeHandler;
	std::function<void()> _errorHandler;

public:
	TcpConnection() = delete;
	TcpConnection(const TcpConnection&) = delete;
	void operator= (TcpConnection const&) = delete;

	TcpConnection(std::shared_ptr<Transport>& transport, int fd, const sockaddr_in &cliaddr, bool outgoing);
	virtual ~TcpConnection();

	const std::chrono::steady_clock::time_point& aTime() const
	{
		return _accessTime;
	}

	bool noRead() const
	{
		return _noRead;
	}

	virtual void watch(epoll_event &ev);

	virtual bool processing();

	void addCompleteHandler(std::function<void(const std::shared_ptr<Context>&)>);
	void addErrorHandler(std::function<void()>);

	void complete()
	{
		_completeHandler(_context);
	}

	void close();
};
