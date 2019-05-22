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

// SslAcceptor.hpp


#pragma once

#include "TcpAcceptor.hpp"

#include <openssl/ssl.h>

class SslAcceptor : public TcpAcceptor
{
private:
	std::shared_ptr<SSL_CTX> _sslContext;

public:
	SslAcceptor() = delete;
	SslAcceptor(const SslAcceptor&) = delete;
	SslAcceptor& operator=(const SslAcceptor&) = delete;
	SslAcceptor(SslAcceptor&& tmp) noexcept = delete;
	SslAcceptor& operator=(SslAcceptor&& tmp) noexcept = delete;

	SslAcceptor(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port, const std::shared_ptr<SSL_CTX>& sslContext);
	~SslAcceptor() override = default;

	void createConnection(int sock, const sockaddr_in& cliaddr) override;

	static std::shared_ptr<Connection> create(const std::shared_ptr<ServerTransport>& transport, const std::string& host, std::uint16_t port, const std::shared_ptr<SSL_CTX>& sslContext)
	{
		return std::make_shared<SslAcceptor>(transport, host, port, sslContext);
	}
};
