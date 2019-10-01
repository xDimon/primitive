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
// File created on: 2017.04.30

// AcceptorFactory.cpp


#include "../configs/Setting.hpp"
#include "Acceptor.hpp"
#include "SslAcceptor.hpp"
#include "../utils/SslHelper.hpp"

std::shared_ptr<AcceptorFactory::Creator> AcceptorFactory::creator(const Setting& setting)
{
	std::string host;
	try
	{
		host = setting.getAs<SStr>("host");
		if (host.empty())
		{
			throw std::runtime_error("Empty value");
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't get host ← " + exception.what());
	}

	uint16_t port;
	try
	{
		int port0 = setting.getAs<SInt>("port");
		if (port0 <= 0 || port0 > 0xFFFF)
		{
			throw std::runtime_error("Wrong value");
		}
		port = static_cast<uint16_t>(port0);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't get port ← " + exception.what());
	}

	bool secure = false;
	if (setting.has("secure"))
	{
		setting.lookup("secure", secure);
	}

	if (secure)
	{
		return std::make_shared<Creator>(
			[host = std::move(host), port](const std::shared_ptr<ServerTransport>& transport)
			{
				return SslAcceptor::create(transport, host, port, SslHelper::getServerContext());
			}
		);
	}
	else
	{
		return std::make_shared<Creator>(
			[host = std::move(host), port](const std::shared_ptr<ServerTransport>& transport)
			{
				return TcpAcceptor::create(transport, host, port);
			}
		);
	}
}
