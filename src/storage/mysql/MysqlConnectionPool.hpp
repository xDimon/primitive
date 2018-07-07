// Copyright © 2017-2018 Dmitriy Khaustov
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

// MysqlConnectionPool.hpp


#pragma once

#include "../DbConnectionPool.hpp"

#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <deque>

class MysqlConnectionPool final : public DbConnectionPool
{
private:
	std::string _dbname;
	std::string _dbsocket;
	std::string _dbserver;
	std::string _dbuser;
	std::string _dbpass;
	unsigned int _dbport;

	std::shared_ptr<DbConnection> create() override;

	DECLARE_DBCONNECTIONPOOL(MysqlConnectionPool)
};
