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

// DbConnection.cpp


#include "DbConnection.hpp"

size_t DbConnection::_lastId = 0;

DbConnection::DbConnection(const std::shared_ptr<DbConnectionPool>& pool)
: id(++_lastId)
, _captured(0)
, _pool(pool)
{
	if (_pool.expired())
	{
		throw std::runtime_error("Attempt create DbConnection with wrong pool");
	}

	pool->metricSumConnections->addValue();
	pool->metricCurrenConnections->addValue();
}

DbConnection::~DbConnection() //override
{
	if (_captured > 0)
	{
//		throw std::runtime_error("Destroy of captured connection ");
	}

	auto pool = _pool.lock();
	if (pool) pool->metricCurrenConnections->addValue(-1);
}
