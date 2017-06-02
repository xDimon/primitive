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
// File created on: 2017.05.08

// DbConnectionPool.cpp


#include <sstream>
#include "DbConnectionPool.hpp"
#include "DbConnection.hpp"

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

	_log.debug("DbConnectionPool '%s' created", _name.c_str());
}

void DbConnectionPool::touch()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_captured.empty() && _pool.empty())
	{
		try
		{
			auto conn = create();
			_pool.push_front(conn);
		}
		catch(...)
		{
			throw;
		}
	}
}

std::shared_ptr<DbConnection> DbConnectionPool::Capture()
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

			conn->capture();

			_captured.insert(std::make_pair(std::this_thread::get_id(), conn));

			conn.reset();
		}

		_mutex.lock();

		// Если соединение никем не захвачено - уничтожаем его
		if (!conn->captured())
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
		if (!conn->captured())
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

void DbConnectionPool::Release(std::shared_ptr<DbConnection> conn)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto i = _captured.find(std::this_thread::get_id());
	if (i == _captured.end())
	{
		return;
	}

	if (conn->captured())
	{
		conn->release();
	}
	else
	{
//		Log::warning("Internal error: release connection when counter of capture is zero");
	}

	// После высвобождения соединение все еще захвачено (согласно счетчику)
	if (conn->captured())
	{
		return;
	}

	if (conn->inTransaction())
	{
//		Log::warningf("Internal error: finaly release connection when counter of incompleted transaction is '%lu'", (*conn)->transaction);

		while (conn->inTransaction())
		{
			conn->rollback();
		}
	}

	_captured.erase(i);

	_pool.push_front(conn);

	return;
}
