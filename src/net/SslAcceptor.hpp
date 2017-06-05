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

// SslAcceptor.hpp


#pragma once

#include "TcpAcceptor.hpp"

#include <openssl/ssl.h>

class SslAcceptor : public TcpAcceptor
{
private:
	std::string _name;
	std::shared_ptr<SSL_CTX>& _sslContext;

public:
	SslAcceptor(std::shared_ptr<ServerTransport>& transport, std::string host, std::uint16_t port, std::shared_ptr<SSL_CTX>& sslContext);
	virtual ~SslAcceptor() {};

	virtual void createConnection(int sock, const sockaddr_in &cliaddr);

	static std::shared_ptr<Connection> create(std::shared_ptr<ServerTransport>& transport, std::string host, std::uint16_t port, std::shared_ptr<SSL_CTX>& sslContext)
	{
		return std::make_shared<SslAcceptor>(transport, host, port, sslContext);
	}
};
