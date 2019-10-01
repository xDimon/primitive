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
// File created on: 2017.03.09

// TcpAcceptor.hpp


#pragma once

#include <mutex>
#include <netinet/in.h>
#include "Acceptor.hpp"

class TcpAcceptor : public Acceptor
{
protected:
	std::string _host;
	std::uint16_t _port;
	std::mutex _mutex;

public:
	TcpAcceptor() = delete;
	TcpAcceptor(const TcpAcceptor&) = delete;
	TcpAcceptor& operator=(const TcpAcceptor&) = delete;
	TcpAcceptor(TcpAcceptor&& tmp) noexcept = delete;
	TcpAcceptor& operator=(TcpAcceptor&& tmp) noexcept = delete;

	TcpAcceptor(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port);
	~TcpAcceptor() override;

	void watch(epoll_event &ev) override;

	virtual void createConnection(int sock, const struct sockaddr& cliaddr);

	bool processing() override;

	static std::shared_ptr<TcpAcceptor> create(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port)
	{
		return std::make_shared<TcpAcceptor>(transport, host, port);
	}
};
