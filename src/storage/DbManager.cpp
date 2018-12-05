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
// File created on: 2017.06.15

// DbManager.cpp


#include "DbManager.hpp"

std::shared_ptr<DbConnectionPool> DbManager::openPool(const Setting& setting, bool replace)
{
	std::string name;
	try
	{
		setting.lookupValue("name", name);
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		throw std::runtime_error("Bad config or name undefined");
	}

	if (!replace)
	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		if (getInstance()._pools.find(name) != getInstance()._pools.end())
		{
			throw std::runtime_error("Already exists database connection pool with the same name ('" + name + "')");
		}
	}

	auto dbPool = DbConnectionPoolFactory::create(setting);

	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
	if (!replace)
	{
		auto i = getInstance()._pools.find(name);
		if (i != getInstance()._pools.end())
		{
			getInstance()._pools.erase(i);
		}
	}

	getInstance()._pools.emplace(dbPool->name(), dbPool);

	dbPool->touch();

	return dbPool;
}

std::shared_ptr<DbConnectionPool> DbManager::getPool(const std::string& name)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	const auto& i = getInstance()._pools.find(name);
	if (i == getInstance()._pools.end())
	{
		return nullptr;
	}

	return i->second;
}

void DbManager::closePool(const std::string& name, bool force)
{
	std::shared_ptr<DbConnectionPool> dbPool;
	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

		auto i = getInstance()._pools.find(name);
		if (i == getInstance()._pools.end())
		{
			return;
		}

		dbPool = i->second;

		getInstance()._pools.erase(i);
	}

	if (force)
	{
		dbPool->close();
	}
}

Db DbManager::getConnection(const std::string& name)
{
	auto pool = getPool(name);

	return Db(pool);
}

void DbManager::forEach(const std::function<void(const std::shared_ptr<DbConnectionPool>&)>& handler)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	for (auto i : getInstance()._pools)
	{
		handler(i.second);
	}
}
