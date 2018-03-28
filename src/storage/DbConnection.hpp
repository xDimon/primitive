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

// DbConnection.hpp


#pragma once

#include "DbConnectionPool.hpp"

#include <string>
#include <memory>
#include <mysql.h>

class DbResult;

class DbConnectionPool;

class DbConnection : public Shareable<DbConnection>
{
private:
	static size_t _lastId;
	const size_t id;
	size_t _captured;

protected:
	std::weak_ptr<DbConnectionPool> _pool;

public:
	DbConnection(const DbConnection&) = delete;
	DbConnection& operator=(DbConnection const&) = delete;
	DbConnection(DbConnection&&) noexcept = delete;
	DbConnection& operator=(DbConnection&&) noexcept = delete;

	explicit DbConnection(const std::shared_ptr<DbConnectionPool>& pool)
	: id(++_lastId)
	, _captured(0)
	, _pool(pool)
	{
		if (_pool.expired())
		{
			throw std::runtime_error("Attempt create DbConnection with wrong pool");
		}
	}

	~DbConnection() //override
	{
		if (_captured > 0)
		{
//			throw std::runtime_error("Destroy of captured connection ");
		}
	}

	size_t captured() const
	{
		return _captured;
	}

	size_t capture()
	{
		return ++_captured;
	}

	size_t release()
	{
		_captured--;
		if (_captured == 0)
		{
			auto pool = _pool.lock();
			if (pool)
			{
				pool->releaseDbConnection(ptr());
			}
		}
		return _captured;
	}

	virtual std::string escape(const std::string& str) = 0;

	virtual bool alive() = 0;

	virtual bool startTransaction() = 0;
	virtual bool deadlockDetected() = 0;
	virtual bool inTransaction() = 0;

	virtual bool commit() = 0;
	virtual bool rollback() = 0;

	inline bool query(const std::string& sql) __attribute_warn_unused_result__
	{
		return query(sql, nullptr, nullptr, nullptr);
	}
	inline bool query(const std::string& sql, DbResult* res) __attribute_warn_unused_result__
	{
		return query(sql, res, nullptr, nullptr);
	}
	inline bool query(const std::string& sql, DbResult* res, size_t* affected) __attribute_warn_unused_result__
	{
		return query(sql, res, affected, nullptr);
	}
	virtual bool query(const std::string& sql, DbResult* res, size_t* affected, size_t* insertId) __attribute_warn_unused_result__ = 0;

	virtual bool multiQuery(const std::string& sql) __attribute_warn_unused_result__ = 0;
};
