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
// File created on: 2017.06.15

// DbConnectionPoolFactory.hpp


#pragma once


#include "DbConnectionPool.hpp"

class DbConnectionPoolFactory
{
private:
	DbConnectionPoolFactory() {};

	virtual ~DbConnectionPoolFactory() {};

	DbConnectionPoolFactory(DbConnectionPoolFactory const&) = delete;

	void operator=(DbConnectionPoolFactory const&) = delete;

	static DbConnectionPoolFactory& getInstance()
	{
		static DbConnectionPoolFactory instance;
		return instance;
	}

	std::map<std::string, std::shared_ptr<DbConnectionPool>(*)(const Setting&)> _creators;

public:
	static bool reg(const std::string& type, std::shared_ptr<DbConnectionPool>(*)(const Setting&));
	static std::shared_ptr<DbConnectionPool> create(const Setting& setting);
};