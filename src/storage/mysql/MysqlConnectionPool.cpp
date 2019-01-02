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

REGISTER_DBCONNECTIONPOOL(mysql, MysqlConnectionPool)

MysqlConnectionPool::MysqlConnectionPool(const Setting& setting)
: DbConnectionPool(setting)
, _dbport(0)
{
	if (!MysqlLibHelper::isReady())
	{
		throw std::runtime_error("MySQL library isn't ready");
	}

	if (setting.exists("dbsocket"))
	{
		setting.lookupValue("dbsocket", _dbsocket);
	}
	else if (setting.exists("dbserver"))
	{
		setting.lookupValue("dbserver", _dbserver);

		if (setting.exists("dbport"))
		{
			setting.lookupValue("dbport", _dbport);
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

	if (setting.exists("dbname"))
	{
		setting.lookupValue("dbname", _dbname);
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbname for dbpool '") + name() + "'");
	}

	if (setting.exists("dbuser"))
	{
		setting.lookupValue("dbuser", _dbuser);
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbuser for dbpool '") + name() + "'");
	}

	if (setting.exists("dbpass"))
	{
		setting.lookupValue("dbpass", _dbpass);
	}
	else
	{
		throw std::runtime_error(std::string("Undefined dbpass for dbpool '") + name() + "'");
	}

	if (setting.exists("dbcharset"))
	{
		setting.lookupValue("dbcharset", _charset);
	}

	if (setting.exists("dbtimezone"))
	{
		setting.lookupValue("dbtimezone", _timezone);
	}
}

std::shared_ptr<DbConnection> MysqlConnectionPool::create()
{
	return std::make_shared<MysqlConnection>(
		ptr(),
		_dbname,
		_dbuser,
		_dbpass,
		_dbserver,
		_dbport,
		_dbsocket,
		_charset,
		_timezone
	);
}

void MysqlConnectionPool::close()
{
	// TODO реализовать закрытие
}
