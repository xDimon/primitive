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

// TcpAcceptor.hpp


#pragma once

#include <mutex>
#include "Connection.hpp"

struct sockaddr_in;

class TcpAcceptor : public Connection
{
private:
	std::string _host;
	std::uint16_t _port;
	std::mutex _mutex;

public:
	TcpAcceptor(Transport::Ptr transport, std::string host, std::uint16_t port);
	virtual ~TcpAcceptor();

	virtual const std::string& name();

	virtual void watch(epoll_event &ev);

	virtual bool processing();
};
