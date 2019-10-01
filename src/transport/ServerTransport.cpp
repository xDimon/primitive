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
// File created on: 2017.02.26

// ServerTransport.cpp


#include <sstream>
#include "ServerTransport.hpp"
#include "../net/ConnectionManager.hpp"
#include "../thread/ThreadPool.hpp"
#include "../telemetry/TelemetryManager.hpp"

ServerTransport::ServerTransport(const std::string& name, const Setting& setting)
{
	_name = name;

	if (_name.empty())
	{
		throw std::runtime_error("Empty name");
	}

	_acceptorCreator = AcceptorFactory::creator(setting);

	metricConnectCount = TelemetryManager::metric("transport/" + _name + "/connections", 1);
	metricRequestCount = TelemetryManager::metric("transport/" + _name + "/requests", 1);
	metricAvgRequestPerSec = TelemetryManager::metric("transport/" + _name + "/requests_per_second", std::chrono::seconds(15));
	metricAvgExecutionTime = TelemetryManager::metric("transport/" + _name + "/requests_exec_time", std::chrono::seconds(15));
}

const sockaddr& ServerTransport::address() const
{
	static const sockaddr nulladdr{};
	auto acceptor = _acceptor.lock();
	return acceptor ? acceptor->address() : nulladdr;
}

bool ServerTransport::enable()
{
	if (!_acceptor.expired())
	{
		_log.trace("Transport '%s' already enabled", name().c_str());
		return true;
	}

	try
	{
		auto acceptor = (*_acceptorCreator)(ptr());

		_acceptor = acceptor->ptr();

		ThreadPool::hold();

		ConnectionManager::add(_acceptor.lock());

		_log.debug("Transport '%s' is enabled", name().c_str());

		return true;
	}
	catch (const std::runtime_error& exception)
	{
		_log.warn("Transport '%s' didn't enable ← Can't create Acceptor ← %s", name().c_str(), exception.what());

		return false;
	}
}

bool ServerTransport::disable()
{
	if (_acceptor.expired())
	{
		_log.trace("Transport '%s' wasn't enabled", name().c_str());
		return true;
	}

	ConnectionManager::remove(_acceptor.lock());

	ThreadPool::unhold();

	_acceptor.reset();

	_log.debug("Transport '%s' is disabled", name().c_str());
	return true;
}
