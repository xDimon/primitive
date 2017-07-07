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
// File created on: 2017.06.15

// DbManager.hpp


#pragma once


#include "DbConnectionPool.hpp"
#include "Db.hpp"

#include <mutex>

class DbManager final
{
private:
	DbManager() {};
	virtual ~DbManager() {};

	DbManager(DbManager const&) = delete;
	void operator= (DbManager const&) = delete;

	static DbManager &getInstance()
	{
		static DbManager instance;
		return instance;
	}

	std::map<std::string, const std::shared_ptr<DbConnectionPool>> _pools;
	std::mutex _mutex;

public:
	static std::shared_ptr<DbConnectionPool> openPool(const Setting& setting, bool replace = false);

	static std::shared_ptr<DbConnectionPool> getPool(const std::string& name);

	static void closePool(const std::string& name, bool force);

	static Db getConnection(const std::string& name);
};
