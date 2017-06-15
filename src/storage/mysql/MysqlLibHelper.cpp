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

// MysqlLibHelper.cpp


#include "MysqlLibHelper.hpp"

#include <mysql.h>
#include <stdexcept>

MysqlLibHelper::MysqlLibHelper()
{
	if (mysql_library_init(0, nullptr, nullptr))
	{
		_ready = false;
		throw std::runtime_error("Could not initialize MySQL library");
	}
	_ready = true;
}

MysqlLibHelper::~MysqlLibHelper()
{
	mysql_library_end();
}
