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

// MysqlConnection.hpp


#pragma once

#include "../DbConnection.hpp"
#include "../../utils/Shareable.hpp"
#include "../../net/TcpConnection.hpp"
#include <memory>
#include <mysql.h>

class MysqlConnectionPool;

class MysqlConnection final : public DbConnection
{
private:
	mutable	MYSQL* _mysql;

	size_t _transaction;

public:
	MysqlConnection() = delete;
	MysqlConnection(const MysqlConnection&) = delete;
	MysqlConnection& operator=(const MysqlConnection&) = delete;
	MysqlConnection(MysqlConnection&&) noexcept = delete;
	MysqlConnection& operator=(MysqlConnection&&) noexcept = delete;

	MysqlConnection(const std::shared_ptr<DbConnectionPool>& pool);

	~MysqlConnection() override;

	bool connect(
		const std::string& dbname,
		const std::string& dbuser,
		const std::string& dbpass,
		const std::string& dbserver,
		unsigned int dbport,
		const std::string& dbsocket,
		const std::string& charset,
		const std::string& timezone
	);

	std::string escape(const std::string& str) override;

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
