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

// DbConnectionPool.hpp


#pragma once


#include <mutex>
#include <thread>
#include <map>
#include <deque>
#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"
#include "../configs/Setting.hpp"
#include "../log/Log.hpp"
#include "../telemetry/Metric.hpp"

class DbConnection;

class DbConnectionPool : public Shareable<DbConnectionPool>, public Named
{
protected:
	Log _log;
	std::recursive_mutex _mutex;

	std::map<std::thread::id, std::shared_ptr<DbConnection>> _captured;
	std::deque<std::shared_ptr<DbConnection>> _pool;

	virtual std::shared_ptr<DbConnection> create() = 0;

public:
	DbConnectionPool() = delete;
	DbConnectionPool(const DbConnectionPool&) = delete;
	void operator=(DbConnectionPool const&) = delete;
	DbConnectionPool(DbConnectionPool&&) = delete;
	DbConnectionPool& operator=(DbConnectionPool&&) = delete;

	explicit DbConnectionPool(const Setting& setting);
	~DbConnectionPool() override = default;

	std::shared_ptr<Metric> metricConnectCount;
	std::shared_ptr<Metric> metricSuccessQueryCount;
	std::shared_ptr<Metric> metricFailQueryCount;
	std::shared_ptr<Metric> metricAvgQueryPerSec;
	std::shared_ptr<Metric> metricAvgExecutionTime;

	void touch();

	std::shared_ptr<DbConnection> captureDbConnection();

	void releaseDbConnection(const std::shared_ptr<DbConnection>& conn);

	virtual void close() = 0;
};

#include "DbConnectionPoolFactory.hpp"

#define REGISTER_DBCONNECTIONPOOL(Type,Class) const bool Class::__dummy = \
    DbConnectionPoolFactory::reg(                                                               \
        #Type,                                                                                  \
        [](const Setting& setting){                                                             \
            return std::shared_ptr<DbConnectionPool>(new Class(setting));                       \
        }                                                                                       \
    );

#define DECLARE_DBCONNECTIONPOOL(Class) \
public:                                                                                         \
	Class() = delete;                                                                           \
	Class(const Class&) = delete;                                                               \
    void operator=(Class const&) = delete;                                                      \
	Class(Class&&) = delete;                                                                    \
	Class& operator=(Class&&) = delete;                                                         \
    ~Class() override = default;                                                                \
                                                                                                \
private:                                                                                        \
    explicit Class(const Setting& setting);                                                     \
																								\
public:                                                                                         \
	void close() override;                                                                      \
                                                                                                \
private:                                                                                        \
    static const bool __dummy;
