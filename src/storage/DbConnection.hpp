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

// DbConnection.hpp


#pragma once

#include "DbConnectionPool.hpp"

#include <string>
#include <memory>
#include <mysql.h>

class DbResult;

class DbConnectionPool;

class DbConnection
{
private:
	static size_t _lastId;
	const size_t id;
	size_t _captured;

protected:
	std::weak_ptr<DbConnectionPool> _pool;

public:
	DbConnection(std::shared_ptr<DbConnectionPool> pool)
	: id(++_lastId)
	, _captured(0)
	, _pool(pool)
	{};

	virtual ~DbConnection()
	{};

	bool captured() const
	{
		return _captured != 0;
	}

	void capture()
	{
		_captured++;
	}

	void release()
	{
		_captured--;
	}

	virtual bool alive() = 0;

	virtual bool startTransaction() = 0;
	virtual bool deadlockDetected() = 0;
	virtual bool inTransaction() = 0;

	virtual bool commit() = 0;
	virtual bool rollback() = 0;

	virtual bool query(const std::string& query, DbResult* res = nullptr, size_t* affected = nullptr, size_t* insertId = nullptr) = 0;
	virtual bool multiQuery(const std::string& sql) = 0;
};
