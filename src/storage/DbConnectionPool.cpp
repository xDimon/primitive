// Copyright © 2017-2018 Dmitriy Khaustov
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
// File created on: 2017.05.08

// DbConnectionPool.cpp


#include <sstream>
#include "DbConnectionPool.hpp"
#include "DbConnection.hpp"
#include "../telemetry/TelemetryManager.hpp"

DbConnectionPool::DbConnectionPool(const Setting& setting)
: _log("DbConnectionPool")
{
	std::string name;
	if (setting.exists("name"))
	{
		setting.lookupValue("name", name);
	}
	else
	{
		std::ostringstream ss(name);
		ss << "_db_pool#" << this;
	}

	_name = std::move(name);

	metricSumConnections = TelemetryManager::metric("db/" + _name + "/connections/count", 1);
	metricCurrenConnections = TelemetryManager::metric("db/" + _name + "/connections/sum", 1);
	metricSuccessQueryCount = TelemetryManager::metric("db/" + _name + "/queries", 1);
	metricFailQueryCount = TelemetryManager::metric("db/" + _name + "/errors", 1);
	metricAvgQueryPerSec = TelemetryManager::metric("db/" + _name + "/queries_per_second", std::chrono::seconds(15));
	metricAvgExecutionTime = TelemetryManager::metric("db/" + _name + "/queries_exec_time", std::chrono::seconds(15));

	_log.debug("DbConnectionPool '%s' created", _name.c_str());
}

void DbConnectionPool::touch()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_captured.empty() && _pool.empty())
	{
		auto conn = create();
		_pool.push_front(conn);
	}
}

std::shared_ptr<DbConnection> DbConnectionPool::captureDbConnection()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto i = _captured.find(std::this_thread::get_id());
	if (i != _captured.end())
	{
		auto conn = std::move(i->second);

		_captured.erase(i);

		_mutex.unlock();

		if (conn->alive())
		{
			_mutex.lock();

			_captured.insert(std::make_pair(std::this_thread::get_id(), conn));

			conn->capture();

			return std::move(conn);
		}

		_mutex.lock();

		// Если соединение никем не захвачено - уничтожаем его
		if (conn->captured() == 0)
		{
			conn.reset();
		}
	}

	while (!_pool.empty())
	{
		auto conn = std::move(_pool.front());
		_pool.pop_front();

		_mutex.unlock();

		if (conn->alive())
		{
			_mutex.lock();

			conn->capture();

			_captured.insert(std::make_pair(std::this_thread::get_id(), conn));

			return std::move(conn);
		}

		_mutex.lock();

		// Если соединение никем не захвачено - уничтожаем его
		if (conn->captured() == 0)
		{
			conn.reset();
		}
	}

	try
	{
		auto conn = create();

		conn->capture();

		_captured.insert(std::make_pair(std::this_thread::get_id(), conn));

		return std::move(conn);
	}
	catch (...)
	{
		return nullptr;
	}
}

void DbConnectionPool::releaseDbConnection(const std::shared_ptr<DbConnection>& conn)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto i = _captured.find(std::this_thread::get_id());
	if (i == _captured.end())
	{
		return;
	}

	if (conn->inTransaction())
	{
		_log.warn("Internal error: finaly release connection when counter of incompleted transaction nozero");

		while (conn->inTransaction())
		{
			conn->rollback();
		}
	}

	_captured.erase(i);

	_pool.push_front(conn);
}
