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

// MysqlConnection.cpp


#include <cstring>
#include "MysqlConnection.hpp"
#include "MysqlConnectionPool.hpp"
#include "MysqlResult.hpp"

MysqlConnection::MysqlConnection(
	const std::shared_ptr<DbConnectionPool>& pool,
	const std::string& dbname,
	const std::string& dbuser,
	const std::string& dbpass,
	const std::string& dbserver,
	unsigned int dbport
)
: DbConnection(std::dynamic_pointer_cast<MysqlConnectionPool>(pool))
, _mysql(nullptr)
, _transaction(0)
{
	my_bool on = (my_bool)0;
	unsigned int timeout = 5;

	_mysql = mysql_init(_mysql);
	if (!_mysql)
	{
		throw std::runtime_error("Can't init mysql connection");
	}

	mysql_options(_mysql, MYSQL_OPT_RECONNECT, &on);
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
	mysql_options(_mysql, MYSQL_INIT_COMMAND, "SET time_zone='+00:00';\n");

	if (!mysql_real_connect(
		_mysql,
		dbserver.c_str(),
		dbuser.c_str(),
		dbpass.c_str(),
		dbname.c_str(),
		dbport,
		nullptr,
		CLIENT_REMEMBER_OPTIONS
	))
	{
		throw std::runtime_error(std::string("Can't connect to database ← ") + mysql_error(_mysql));
	}

	timeout = 900;
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
}

MysqlConnection::MysqlConnection(
	const std::shared_ptr<DbConnectionPool>& pool,
	const std::string& dbname,
	const std::string& dbuser,
	const std::string& dbpass,
	const std::string& dbsocket
)
: DbConnection(pool)
, _mysql(nullptr)
, _transaction(0)
{
	my_bool on = (my_bool)0;
	unsigned int timeout = 5;

	_mysql = mysql_init(_mysql);
	if (!_mysql)
	{
		throw std::runtime_error("Can't init mysql connection");
	}

	mysql_options(_mysql, MYSQL_OPT_RECONNECT, &on);
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
	mysql_options(_mysql, MYSQL_INIT_COMMAND, "SET time_zone='+00:00';\n");

	if (!mysql_real_connect(
		_mysql,
		nullptr,
		dbuser.c_str(),
		dbpass.c_str(),
		dbname.c_str(),
		0,
		dbsocket.c_str(),
		CLIENT_REMEMBER_OPTIONS
	))
	{
		throw std::runtime_error(std::string("Can't connect to database ← ") + mysql_error(_mysql));
	}

	timeout = 900;
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
}

MysqlConnection::~MysqlConnection()
{
	mysql_close(_mysql);
}

bool MysqlConnection::startTransaction()
{
	if (_transaction)
	{
		_transaction++;
		return true;
	}

	if (query("START TRANSACTION;"))
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
		Log("mysql").warn("Internal error: commit when counter of transaction is zero");
		return false;
	}
	if (query("COMMIT;"))
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
	if (!_transaction)
	{
		Log("mysql").warn("Internal error: rollback when counter of transaction is zero");
		return false;
	}
	if (query("ROLLBACK;"))
	{
		_transaction = 0;
		return true;
	}

	return false;
}

bool MysqlConnection::query(const std::string& sql, DbResult* res, size_t* affected, size_t* insertId)
{
	auto result = dynamic_cast<MysqlResult *>(res);

	// Выполнение запроса
	if (mysql_query(_mysql, sql.c_str()) != 0)
	{
		Log("mysql").error("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
		return false;
	}

	if (affected)
	{
		*affected = mysql_affected_rows(_mysql);
	}

	if (insertId)
	{
		*insertId = mysql_insert_id(_mysql);
	}

	if (res != nullptr)
	{
		result->set(mysql_store_result(_mysql));
		if (!result->get() && mysql_errno(_mysql))
		{
			Log("mysql").error("MySQL store result error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
			return false;
		}
	}

	return true;
}

bool MysqlConnection::multiQuery(const std::string& sql)
{
	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);

	// Выполнение запроса
	if (mysql_query(_mysql, sql.c_str()) != 0)
	{
		Log("mysql").error("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());

		mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
		return false;
	}

	do
	{
		MYSQL_RES* result = mysql_store_result(_mysql);
		if (result)
		{
			mysql_free_result(result);
		}
	}
	while (mysql_next_result(_mysql) == 0);

	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	return true;
}
