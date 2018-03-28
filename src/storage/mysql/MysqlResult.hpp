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

// MysqlResult.hpp


#pragma once


#include "../DbResult.hpp"

#include <mysql.h>

class MysqlResult : public DbResult
{
private:
	MYSQL_RES* _data;

public:
	MysqlResult(const MysqlResult&) = delete;
	MysqlResult& operator=(MysqlResult const&) = delete;
	MysqlResult(MysqlResult&&) noexcept = delete;
	MysqlResult& operator=(MysqlResult&&) noexcept = delete;

	MysqlResult()
	: _data(nullptr)
	{
	}

	~MysqlResult() override
	{
		mysql_free_result(_data);
	}

	void set(MYSQL_RES* value)
	{
		_data = value;
	}
	MYSQL_RES* get()
	{
		return _data;
	}
};
