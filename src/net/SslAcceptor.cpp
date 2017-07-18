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

	class TimeoutWatcher: public Shareable<TimeoutWatcher>
	{
	private:
		std::weak_ptr<Connection> _wp;

	public:
		TimeoutWatcher(const std::shared_ptr<Connection>& connection): _wp(connection) {};

		void operator()()
		{
			auto connection = std::dynamic_pointer_cast<TcpConnection>(_wp.lock());
			if (!connection)
			{
				return;
			}
			if (connection->expired())
			{
				Log("Timeout").debug("Connection '%s' closed by timeout", connection->name().c_str());
				connection->close();
			}
			else
			{
				ThreadPool::enqueue(
					std::make_shared<std::function<void()>>(
						[p = ptr()](){
							(*p)();
						}
					),
					connection->expireTime()
				);
			}
		}
	};

	auto tow = std::make_shared<TimeoutWatcher>(newConnection);

	ThreadPool::enqueue(
		std::make_shared<std::function<void()>>(
			[p = tow](){
				(*p)();
			}
		),
		newConnection->expireTime()
	);

	ConnectionManager::add(newConnection->ptr());
}
