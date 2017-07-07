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
	size_t _transaction;

	//mutable
	MYSQL _mysql;

public:
	MysqlConnection(
		std::shared_ptr<DbConnectionPool> pool,
		const std::string& dbname,
		const std::string& dbuser,
		const std::string& dbpass,
		const std::string& dbserver,
		unsigned int dbport
	);

	MysqlConnection(
		std::shared_ptr<DbConnectionPool> pool,
		const std::string& dbname,
		const std::string& dbuser,
		const std::string& dbpass,
		const std::string& dbsocket
	);

	virtual ~MysqlConnection();

	MysqlConnection() = delete;

	void operator=(MysqlConnection const&) = delete;

	virtual std::string escape(const std::string& str) override
	{
		std::string res;
		res.reserve(str.length() * 2);
		for (size_t i = 0; i < str.length(); ++i)
		{
			auto& c = str[i];
			if (c == '\x00' || c == '\n' || c == '\r' || c == '\\' || c == '\'' || c == '"' || c == '\x1A')
			{
				res.push_back('\\');
			}
			res.push_back(c);
		}
		return std::move(res);
	}

	virtual bool alive() override
	{
		return !mysql_ping(&_mysql);
	}

	virtual bool startTransaction() override;

	virtual bool deadlockDetected() override;

	virtual bool inTransaction() override
	{
		return _transaction > 0;
	}

	virtual bool commit() override;

	virtual bool rollback() override;

	virtual bool query(const std::string& query, DbResult* res = nullptr, size_t* affected = nullptr, size_t* insertId = nullptr) override;
	virtual bool multiQuery(const std::string& sql) override;
};
