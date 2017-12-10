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
// File created on: 2017.06.02

// Db.hpp


#pragma once


#include "DbConnection.hpp"

class Db final
{
private:
	std::shared_ptr<DbConnection> _conn;

public:
	explicit Db(const std::shared_ptr<DbConnectionPool>& dbPool)
	: _conn(dbPool ? dbPool->captureDbConnection() : nullptr)
	{
		if (!_conn)
		{
			throw std::runtime_error("Can't database connect");
		}
	}
	// Copy
	Db(const Db& that)
	: _conn(that._conn)
	{
		if (!_conn)
		{
			throw std::runtime_error("Can't database connect");
		}
		_conn->capture();
	}
	// Move
	Db(Db&& that) noexcept
	: _conn(std::move(that._conn))
	{
	}

	// Copy assignment operator.
	Db& operator=(const Db& that)
	{
		if (_conn)
		{
			_conn->release();
		}
		_conn = that._conn;
		if (!_conn)
		{
			throw std::runtime_error("Can't database connect");
		}
		_conn->capture();
		return *this;
	}
	// Move assignment operator.
	Db& operator=(Db&& that) noexcept
	{
		_conn = std::move(that._conn);
		return *this;
	}

	~Db()
	{
		_conn->release();
	}

	DbConnection* operator->()
	{
		return _conn.get();
	}
};
