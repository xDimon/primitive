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

// MysqlConnection.cpp


#include <cstring>
#include "MysqlConnection.hpp"
#include "MysqlConnectionPool.hpp"
#include "MysqlResult.hpp"
#include "../../thread/Thread.hpp"
#include "../../net/ConnectionManager.hpp"
#include "../../thread/TaskManager.hpp"
#include "../../thread/RollbackStackAndRestoreContext.hpp"

MysqlConnection::MysqlConnection(const std::shared_ptr<DbConnectionPool>& pool)
: DbConnection(std::dynamic_pointer_cast<MysqlConnectionPool>(pool))
, _mysql(nullptr)
, _transaction(0)
{
	_mysql = mysql_init(_mysql);
	if (_mysql == nullptr)
	{
		throw std::runtime_error("Can't init mysql connection");
	}

	unsigned int timeout = 900;
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
}

MysqlConnection::~MysqlConnection()
{
	mysql_close(_mysql);
}

bool MysqlConnection::connect(
	const std::string& dbname,
	const std::string& dbuser,
	const std::string& dbpass,
	const std::string& dbserver,
	unsigned int dbport,
	const std::string& dbsocket,
	const std::string& charset,
	const std::string& timezone
)
{
	my_bool on = true;
	mysql_options(_mysql, MYSQL_OPT_RECONNECT, &on);

	if (!charset.empty())
	{
		mysql_options(_mysql, MYSQL_SET_CHARSET_NAME, charset.c_str());
	}
	if (!timezone.empty())
	{
		mysql_options(_mysql, MYSQL_INIT_COMMAND, ("SET time_zone='" + timezone + "';\n").c_str());
	}

	if (mysql_real_connect(
		_mysql,
		dbserver.c_str(),
		dbuser.c_str(),
		dbpass.c_str(),
		dbname.c_str(),
		dbport,
		dbsocket.c_str(),
		CLIENT_REMEMBER_OPTIONS
	) == nullptr)
	{
		throw std::runtime_error(std::string("Can't connect to database ← ") + mysql_error(_mysql));
		return false;
	}

	return true;
}

std::string MysqlConnection::escape(const std::string& str)
{
	size_t size = str.length() * 4 + 1;
	if (size > 1024)
	{
		auto buff = new char[size];
		mysql_escape_string(buff, str.c_str(), str.length());
		std::string result(buff);
		free(buff);
		return result;
	}
	else
	{
		char buff[size];
		mysql_escape_string(buff, str.c_str(), str.length());
		return std::forward<std::string>(buff);
	}
}

bool MysqlConnection::startTransaction()
{
	if (_transaction > 0)
	{
		_transaction++;
		return true;
	}

	if (DbConnection::query("START TRANSACTION;"))
	{
		_transaction = 1;
		return true;
	}

	return false;
}

bool MysqlConnection::deadlockDetected()
{
	return mysql_errno(_mysql) == 1213;
}

bool MysqlConnection::commit()
{
	if (_transaction > 1)
	{
		_transaction--;
		return true;
	}
	if (_transaction == 0)
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: commit when counter of transaction is zero");
		}
		return false;
	}
	if (DbConnection::query("COMMIT;"))
	{
		_transaction = 0;
		return true;
	}

	return false;
}

bool MysqlConnection::rollback()
{
	if (_transaction > 1)
	{
		_transaction--;
		return true;
	}
	if (_transaction == 0)
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: rollback when counter of transaction is zero");
		}
		return false;
	}
	if (!DbConnection::query("ROLLBACK;"))
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: Fail at rolling back");
		}
	}

	_transaction = 0;
	return true;
}

bool MysqlConnection::query(const std::string& sql, DbResult* res, size_t* affected, size_t* insertId)
{
	auto pool = _pool.lock();

	auto result = dynamic_cast<MysqlResult *>(res);

	auto beginTime = std::chrono::steady_clock::now();
	if (pool) pool->metricAvgQueryPerSec->addValue();

	if (pool) pool->log().debug("MySQL query: %s", sql.c_str());

	// Выполнение запроса
	bool success = mysql_real_query(_mysql, sql.c_str(), sql.length()) == 0;

	auto now = std::chrono::steady_clock::now();
	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(now - beginTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		if (pool) pool->metricAvgExecutionTime->addValue(timeSpent, now);
	}

	if (timeSpent >= 5)
	{
		if (pool) pool->log().warn("MySQL query too long: %0.03f sec\n\t\tFor query:\n\t\t%s", timeSpent, sql.c_str());
	}

	if (!success)
	{
		if (pool) pool->metricFailQueryCount->addValue();
		if (!deadlockDetected())
		{
			if (pool) pool->log().warn("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
		}
		return false;
	}

	if (pool) pool->log().debug("MySQL query success: " + sql);

	if (affected != nullptr)
	{
		*affected = mysql_affected_rows(_mysql);
	}

	if (insertId != nullptr)
	{
		*insertId = mysql_insert_id(_mysql);
	}

	if (result != nullptr)
	{
		result->set(mysql_store_result(_mysql));

		if (result->get() == nullptr && mysql_errno(_mysql) != 0)
		{
			if (pool) pool->metricFailQueryCount->addValue();
			if (pool) pool->log().error("MySQL store result error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
			return false;
		}
	}

	if (pool) pool->metricSuccessQueryCount->addValue();
	return true;
}

bool MysqlConnection::multiQuery(const std::string& sql)
{
	auto pool = _pool.lock();

	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);

	auto beginTime = std::chrono::steady_clock::now();
	if (pool) pool->metricAvgQueryPerSec->addValue();

	// Выполнение запроса
	bool success = mysql_real_query(_mysql, sql.c_str(), sql.length()) == 0;

	auto now = std::chrono::steady_clock::now();
	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(now - beginTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		if (pool) pool->metricAvgExecutionTime->addValue(timeSpent, now);
	}

	if (!success)
	{
		if (pool) pool->log().error("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());

		mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
		if (pool) pool->metricFailQueryCount->addValue();
		return false;
	}

	for(;;)
	{
		MYSQL_RES* result = mysql_store_result(_mysql);
		if (result != nullptr)
		{
			mysql_free_result(result);
		}

		if (mysql_next_result(_mysql) != 0)
		{
			break;
		}
	}

	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	if (pool) pool->metricSuccessQueryCount->addValue();

	return true;
}
