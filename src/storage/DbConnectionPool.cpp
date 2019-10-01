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
// File created on: 2017.05.08

// DbConnectionPool.cpp


#include <sstream>
#include "DbConnectionPool.hpp"
#include "DbConnection.hpp"
#include "../telemetry/TelemetryManager.hpp"
#include "../thread/Thread.hpp"

DbConnectionPool::DbConnectionPool(const Setting& setting)
: _log("DbConnectionPool")
{
	std::string name;
	if (setting.hasOf<SStr>("name"))
	{
		name = setting.getAs<SStr>("name");
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
	std::unique_lock<mutex_t> lock(_mutex);
	if (_captured.empty() && _pool.empty())
	{
		auto conn = create();
		_pool.push_front(conn);
	}
}

std::shared_ptr<DbConnection> DbConnectionPool::captureDbConnection()
{
	std::shared_ptr<DbConnection> conn;

	std::unique_lock<mutex_t> lock(_mutex);

	auto i = _captured.find(Thread::self()->id());
	if (i != _captured.end())
	{
		conn = std::move(i->second);

		_log.trace("Detach #%u (check alive)", conn->id);

		_captured.erase(i);

		lock.unlock();

		if (conn->alive())
		{
			lock.lock();

			_captured.emplace(Thread::self()->id(), conn);

			conn->capture();

			_log.trace("Attach #%u (check alive)", conn->id);

			return conn;
		}

		lock.lock();

		// Если соединение никем не захвачено - уничтожаем его
		if (conn->captured() == 0)
		{
			conn.reset();
		}
	}

	while (!_pool.empty())
	{
		conn = std::move(_pool.front());
		_pool.pop_front();

		lock.unlock();

		if (conn->alive())
		{
			lock.lock();

			_captured.emplace(Thread::self()->id(), conn);

			conn->capture();

			_log.trace("Attach #%u (get from pool)", conn->id);

			return conn;
		}

		lock.lock();

		// Если соединение никем не захвачено - уничтожаем его
		if (conn->captured() == 0)
		{
			conn.reset();
		}
	}

//	try
//	{
		conn = create();

		_captured.emplace(Thread::self()->id(), conn);

		conn->capture();

		_log.trace("Attach #%u (new)", conn->id);

		return conn;
//	}
//	catch (const std::exception& exception)
//	{
//		std::string err(exception.what());
//		return nullptr;
//	}
}

void DbConnectionPool::releaseDbConnection(const std::shared_ptr<DbConnection>& conn)
{
	std::unique_lock<mutex_t> lock(_mutex);

	auto i = _captured.find(Thread::self()->id());
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

	_log.trace("Detach #%u (return into pool)", conn->id);
}

void DbConnectionPool::attachDbConnection(const std::shared_ptr<DbConnection>& conn)
{
	std::unique_lock<mutex_t> lock(_mutex);

	auto i = _captured.find(Thread::self()->id());
	if (i != _captured.end())
	{
		throw std::runtime_error("Thread already has attached database connection");
	}

	_log.trace("Attach #%u", conn->id);

	_captured.emplace(Thread::self()->id(), conn);
}

std::shared_ptr<DbConnection> DbConnectionPool::detachDbConnection()
{
	std::unique_lock<mutex_t> lock(_mutex);

	auto id = Thread::self()->id();
	auto i = _captured.find(id);
	if (i == _captured.end())
	{
		throw std::runtime_error("Thread already has not attached database connection");
	}

	_log.trace("Detach #%u", i->second->id);

	_captured.erase(i);

	return i->second;
}
