// Copyright © 2019 Dmitriy Khaustov
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
// File created on: 2019.03.16

// MysqlAsyncConnection.hpp


#pragma once

#include <mysql.h>

#ifndef MARIADB_BASE_VERSION // https://mariadb.com/kb/en/library/using-the-non-blocking-library/

#include "MysqlConnection.hpp"
using MysqlAsyncConnection = MysqlConnection;

#else // MARIADB_BASE_VERSION

#include "../DbConnection.hpp"
#include "../../utils/Shareable.hpp"
#include "../../net/TcpConnection.hpp"
#include <memory>

class MysqlConnectionPool;

class MysqlAsyncConnection final : public DbConnection
{
private:
	class MysqlAsyncConnectionHelper: public Connection
	{
	private:
		std::weak_ptr<MysqlAsyncConnection> _parent;
	public:
		explicit MysqlAsyncConnectionHelper(const std::shared_ptr<MysqlAsyncConnection>& parent, int sock)
		: Connection(nullptr)
		, _parent(parent)
		{
			_sock = sock;
			if (_sock != -1)
			{
				_closed = false;
			}
			_name = "MysqlConnection[" + std::to_string(_sock) + "]";
		}
		void watch(epoll_event &ev) override;
		bool processing() override;
	};

	mutable	MYSQL* _mysql;

	std::shared_ptr<MysqlAsyncConnectionHelper> _helper;
	size_t _transaction;
	int _status;
	ucontext_t* _ctx;

public:
	MysqlAsyncConnection() = delete;
	MysqlAsyncConnection(const MysqlAsyncConnection&) = delete;
	MysqlAsyncConnection& operator=(const MysqlAsyncConnection&) = delete;
	MysqlAsyncConnection(MysqlAsyncConnection&&) noexcept = delete;
	MysqlAsyncConnection& operator=(MysqlAsyncConnection&&) noexcept = delete;

	explicit MysqlAsyncConnection(const std::shared_ptr<DbConnectionPool>& pool);

	~MysqlAsyncConnection() override;

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

private:

	bool implQuery(const std::string& sql);

	MYSQL_RES* implStoreResult();

	int implNextResult();

	void implFreeResult(MYSQL_RES* result);

	void implWait(bool isNew = false);
};

#endif // MARIADB_BASE_VERSION
