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
// File created on: 2017.03.27

// HttpTransport.cpp


#include "HttpTransport.hpp"

#include "../net/ConnectionManager.hpp"
#include "../net/TcpConnection.hpp"

HttpTransport::HttpTransport(std::string host, uint16_t port)
: Log("HttpTransport")
, _host(host)
, _port(port)
, _acceptor(TcpAcceptor::Ptr())
{
	log().debug("Create transport 'HttpTransport({}:{})'", _host, _port);
}

HttpTransport::~HttpTransport()
{
}

bool HttpTransport::enable()
{
	if (!_acceptor.expired())
	{
		return true;
	}
	log().debug("Enable transport 'HttpTransport({}:{})'", _host, _port);
	try
	{
		auto acceptor = std::shared_ptr<Connection>(new TcpAcceptor(ptr(), _host, _port));

		_acceptor = acceptor->ptr();

		ConnectionManager::add(_acceptor.lock());

		return true;
	}
	catch(std::runtime_error exception)
	{
		log().debug("Can't create Acceptor for transport 'HttpTransport({}:{})': {}", _host, _port, exception.what());

		return false;
	}
}

bool HttpTransport::disable()
{
	if (_acceptor.expired())
	{
		return true;
	}
	log().debug("Disable transport 'HttpTransport({}:{})'", _host, _port);

	ConnectionManager::remove(_acceptor.lock());

	_acceptor.reset();

	return true;
}

bool HttpTransport::processing(std::shared_ptr<Connection> connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	// TODO Implement processing

	return false;
}
