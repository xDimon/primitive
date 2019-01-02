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

static uint32_t id4noname = 0;

ServerTransport::ServerTransport(const Setting& setting)
{
	std::string name;
	if (setting.exists("name"))
	{
		setting.lookupValue("name", name);
	}

	if (name.empty())
	{
		name = "server[" + std::to_string(++id4noname) + "_unknown]";
	}

	_name = std::move(name);

	_acceptorCreator = AcceptorFactory::creator(setting);

	metricConnectCount = TelemetryManager::metric("transport/" + _name + "/connections", 1);
	metricRequestCount = TelemetryManager::metric("transport/" + _name + "/requests", 1);
	metricAvgRequestPerSec = TelemetryManager::metric("transport/" + _name + "/requests_per_second", std::chrono::seconds(15));
	metricAvgExecutionTime = TelemetryManager::metric("transport/" + _name + "/requests_exec_time", std::chrono::seconds(15));
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
		auto t = this->ptr();
		auto acceptor = (*_acceptorCreator)(t);

		_acceptor = acceptor->ptr();

		ThreadPool::hold();

		ConnectionManager::add(_acceptor.lock());

		_log.debug("Transport '%s' enabled", name().c_str());

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
		_log.debug("Transport '%s' not enable", name().c_str());
		return true;
	}

	ConnectionManager::remove(_acceptor.lock());

	ThreadPool::unhold();

	_acceptor.reset();

	_log.debug("Transport '%s' disabled", name().c_str());
	return true;
}
