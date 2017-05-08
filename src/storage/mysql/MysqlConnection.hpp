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

class MysqlConnection : public Shareable<MysqlConnection>, public DbConnection
{
private:
	size_t _transaction;

	mutable MYSQL _mysql;

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

	virtual bool alive() const override
	{
		return !mysql_ping(&_mysql);
	}

	virtual bool startTransaction() override;

	virtual bool deadlockDetected() const override;

	virtual bool inTransaction() const override
	{
		return _transaction > 0;
	}

	virtual bool commit() override;

	virtual bool rollback() override;

	virtual bool query(const std::string& query, DbResult* res = nullptr, int* affected = nullptr, int* insertId = nullptr) override;
	virtual bool multiQuery(const std::string& sql) override;
};
