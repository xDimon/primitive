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
// File created on: 2017.04.29

// TransportFactory.cpp


#include "TransportFactory.hpp"
#include "HttpTransport.hpp"
#include "WsTransport.hpp"
#include "PacketTransport.hpp"
#include "../net/SslAcceptor.hpp"

std::shared_ptr<Transport> TransportFactory::create(const Setting &setting)
{
	std::shared_ptr<AcceptorFactory::Creator> acceptorCreator;
	try
	{
		acceptorCreator = AcceptorFactory::creator(setting);
	}
	catch(...)
	{
		throw;
	}

	std::string type;
	try
	{
		setting.lookupValue("type", type);
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		throw std::runtime_error("Bad config or type undefined");
	}

	if (type == "http")
	{
		return getInstance().createHttp(setting);
	}
	else if (type == "websocket")
	{
		return getInstance().createWs(setting);
	}
	else if (type == "packet")
	{
		return getInstance().createPacket(setting);
	}
	else
	{
		throw std::runtime_error("Unknown transport type");
	}
}

std::shared_ptr<Transport> TransportFactory::createHttp(const Setting &setting)
{
	return std::make_shared<HttpTransport>(setting);
}

std::shared_ptr<Transport> TransportFactory::createWs(const Setting &setting)
{
	return std::make_shared<WsTransport>(setting);
}

std::shared_ptr<Transport> TransportFactory::createPacket(const Setting &setting)
{
	return std::make_shared<PacketTransport>(setting);
}
