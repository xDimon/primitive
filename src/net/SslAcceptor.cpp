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

SslAcceptor::SslAcceptor(Transport::Ptr& transport, std::string host, std::uint16_t port, std::string certificate, std::string key)
: TcpAcceptor(transport, host, port)
{
	BIO *cbio = BIO_new_mem_buf(certificate.data(), -1);
	X509 *cert = PEM_read_bio_X509(cbio, NULL, 0, NULL);
	assert(cert != NULL);

	BIO *kbio = BIO_new_mem_buf(key.data(), -1);
	RSA *rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
	assert(rsa != NULL);

	struct D {
		void operator()(SSL_CTX* ctx) const {
			SSL_CTX_free(ctx);
		}
	};

	_sslContext = std::shared_ptr<SSL_CTX>(SSL_CTX_new(SSLv23_server_method()), D());

	SSL_CTX_set_options(_sslContext.get(), SSL_OP_SINGLE_DH_USE);

	SSL_CTX_use_certificate(_sslContext.get(), cert);
	SSL_CTX_use_RSAPrivateKey(_sslContext.get(), rsa);
}

void SslAcceptor::createConnection(int sock, const sockaddr_in &cliaddr)
{
	Transport::Ptr transport = _transport.lock();

	auto connection = std::shared_ptr<Connection>(new SslConnection(transport, sock, cliaddr, _sslContext));

	ConnectionManager::add(connection->ptr());
}
