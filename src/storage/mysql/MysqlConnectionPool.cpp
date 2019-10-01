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

// MysqlConnectionPool.cpp


#include <sstream>
#include "MysqlConnectionPool.hpp"
#include "MysqlConnection.hpp"
#include "MysqlLibHelper.hpp"
#include "MysqlAsyncConnection.hpp"

REGISTER_DBCONNECTIONPOOL(mysql, MysqlConnectionPool)

MysqlConnectionPool::MysqlConnectionPool(const Setting& setting)
: DbConnectionPool(setting)
, _dbport(0)
, _async(false)
{
	if (!MysqlLibHelper::isReady())
	{
		throw std::runtime_error("MySQL library isn't ready");
	}

	if (setting.hasOf<SStr>("dbsocket"))
	{
		_dbsocket = setting.getAs<SStr>("dbsocket");
	}
	else if (setting.hasOf<SStr>("dbserver"))
	{
		_dbserver = setting.getAs<SStr>("dbserver");

		if (setting.has("dbport"))
		{
			setting.lookup("dbport", _dbport);
		}
		else
		{
			_dbport = 3306;
		}
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbserver/dbsocket for dbpool '") + name() + "'");
	}

	if (setting.hasOf<SStr>("dbname"))
	{
		_dbname = setting.getAs<SStr>("dbname");
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbname for dbpool '") + name() + "'");
	}

	if (setting.hasOf<SStr>("dbuser"))
	{
		_dbuser = setting.getAs<SStr>("dbuser");
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbuser for dbpool '") + name() + "'");
	}

	if (setting.hasOf<SStr>("dbpass"))
	{
		_dbpass = setting.getAs<SStr>("dbpass");
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbpass for dbpool '") + name() + "'");
	}

	if (setting.has("async"))
	{
		setting.lookup("async", _async);
	}

	if (setting.hasOf<SStr>("dbcharset"))
	{
		_charset = setting.getAs<SStr>("dbcharset");
	}

	if (setting.hasOf<SStr>("dbtimezone"))
	{
		_timezone = setting.getAs<SStr>("dbtimezone");
	}
}

std::shared_ptr<DbConnection> MysqlConnectionPool::create()
{
	if (_async)
	{
		auto conn = std::make_shared<MysqlAsyncConnection>(ptr());

		conn->connect(
			_dbname,
			_dbuser,
			_dbpass,
			_dbserver,
			_dbport,
			_dbsocket,
			_charset,
			_timezone
		);

		return conn;
	}
	else
	{
		auto conn = std::make_shared<MysqlConnection>(ptr());

		conn->connect(
			_dbname,
			_dbuser,
			_dbpass,
			_dbserver,
			_dbport,
			_dbsocket,
			_charset,
			_timezone
		);

		return conn;
	}
}

void MysqlConnectionPool::close()
{
	std::unique_lock<mutex_t> lock(_mutex);

	_pool.clear();
	_captured.clear();
}
