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

// SslAcceptor.cpp


#include "SslAcceptor.hpp"
#include "SslConnection.hpp"
#include "ConnectionManager.hpp"

SslAcceptor::SslAcceptor(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port, const std::shared_ptr<SSL_CTX>& context)
: TcpAcceptor(transport, host, port)
, _sslContext(context)
{
	_name = "SslAcceptor" + _name.substr(11);
}

void SslAcceptor::createConnection(int sock, const sockaddr &cliaddr)
{
	auto transport = std::dynamic_pointer_cast<ServerTransport>(_transport.lock());
	if (!transport)
	{
		return;
	}

	auto newConnection = std::make_shared<SslConnection>(transport, sock, cliaddr, _sslContext, false);
	if (!newConnection)
	{
		return;
	}

	newConnection->setTtl(std::chrono::seconds(5));

	ConnectionManager::add(newConnection->ptr());

	transport->metricConnectCount->addValue();
}
