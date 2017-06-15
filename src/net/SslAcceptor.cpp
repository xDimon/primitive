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

// SslAcceptor.cpp


#include "SslAcceptor.hpp"
#include "SslConnection.hpp"
#include "ConnectionManager.hpp"
#include "../thread/ThreadPool.hpp"

SslAcceptor::SslAcceptor(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port, const std::shared_ptr<SSL_CTX>& context)
: TcpAcceptor(transport, host, port)
, _sslContext(context)
{
}

void SslAcceptor::createConnection(int sock, const sockaddr_in &cliaddr)
{
	std::shared_ptr<Transport> transport = _transport.lock();

	auto newConnection = std::shared_ptr<Connection>(new SslConnection(transport, sock, cliaddr, _sslContext, false));

	ThreadPool::enqueue([wp = std::weak_ptr<Connection>(newConnection->ptr())](){
		auto connection = std::dynamic_pointer_cast<TcpConnection>(wp.lock());
		if (!connection)
		{
			return;
		}
		if (std::chrono::steady_clock::now() > connection->aTime() + std::chrono::seconds(10))
		{
			Log("Timeout").debug("Connection '%s' closed by timeout", connection->name().c_str());
			connection->close();
		}
	}, std::chrono::seconds(10));

	ConnectionManager::add(newConnection->ptr());
}
