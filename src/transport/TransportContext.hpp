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
// File created on: 2017.06.27

// TransportContext.hpp


#pragma once


#include "../utils/Context.hpp"
#include "ServerTransport.hpp"

class TransportContext : public Context
{
protected:
	std::weak_ptr<Connection> _connection;
	std::shared_ptr<Transport::Handler> _handler;
	std::shared_ptr<Transport::Transmitter> _transmitter;

public:
	TransportContext(const std::shared_ptr<Connection>& connection)
	: _connection(connection)
	{};
	virtual ~TransportContext()	= default;

	void setTransmitter(const std::shared_ptr<Transport::Transmitter>& transmitter)
	{
		_transmitter = transmitter;
	}

	void setTtl(std::chrono::milliseconds ttl)
	{
		auto connection = _connection.lock();
		if (connection)
		{
			connection->setTtl(ttl);
		}
	}

	void transmit(const char* data, size_t size, const std::string& contentType, bool close)
	{
		if (_transmitter)
		{
			(*_transmitter)(data, size, contentType, close);
		}
	}
	void transmit(const std::string& data, const std::string& contentType, bool close)
	{
		transmit(data.c_str(), data.length(), contentType, close);
	}
	void transmit(const std::vector<char>& data, const std::string& contentType, bool close)
	{
		transmit(data.data(), data.size(), contentType, close);
	}

	void setHandler(const std::shared_ptr<Transport::Handler>& handler)
	{
		_handler = handler;
	}

	void handle()
	{
		if (_handler)
		{
			(*_handler)(ptr());
		}
	}
};
