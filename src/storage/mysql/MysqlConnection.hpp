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

// MysqlConnection.hpp


#pragma once

#include "../DbConnection.hpp"
#include "../../utils/Shareable.hpp"
#include <memory>
#include <mysql.h>

class MysqlConnectionPool;

class MysqlConnection : public DbConnection
{
private:
	mutable	MYSQL* _mysql;

	size_t _transaction;

public:
	MysqlConnection() = delete;
	MysqlConnection(const MysqlConnection&) = delete;
	void operator=(MysqlConnection const&) = delete;
	MysqlConnection(MysqlConnection&&) = delete;
	MysqlConnection& operator=(MysqlConnection&&) = delete;

	MysqlConnection(
		const std::shared_ptr<DbConnectionPool>& pool,
		const std::string& dbname,
		const std::string& dbuser,
		const std::string& dbpass,
		const std::string& dbserver,
		unsigned int dbport
	);

	MysqlConnection(
		const std::shared_ptr<DbConnectionPool>& pool,
		const std::string& dbname,
		const std::string& dbuser,
		const std::string& dbpass,
		const std::string& dbsocket
	);

	~MysqlConnection() override;

	std::string escape(const std::string& str) override
	{
		std::string res;
		res.reserve(str.length() * 2);
		for (char c : str)
		{
			if (c == '\x00' || c == '\n' || c == '\r' || c == '\\' || c == '\'' || c == '"' || c == '\x1A')
			{
				res.push_back('\\');
			}
			res.push_back(c);
		}
		return std::move(res);
	}

	bool alive() override
	{
		return mysql_ping(_mysql) == 0;
	}

	bool startTransaction() override;

	bool deadlockDetected() override;

	bool inTransaction() override
	{
		return _transaction > 0;
	}

	bool commit() override;

	bool rollback() override;

	bool query(const std::string& query, DbResult* res, size_t* affected, size_t* insertId) override;
	bool multiQuery(const std::string& sql) override;
};
