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
// File created on: 2017.03.29

// WsTransport.hpp


#pragma once

#include <string>
#include "../net/TcpAcceptor.hpp"
#include "Transport.hpp"

class WsTransport : public Transport
{
private:
	std::string _host;
	uint16_t _port;

	TcpAcceptor::WPtr _acceptor;

public:
	WsTransport(std::string host, uint16_t port);

	virtual ~WsTransport();

	virtual bool enable();
	virtual bool disable();

	virtual bool processing(std::shared_ptr<Connection> connection);
};
