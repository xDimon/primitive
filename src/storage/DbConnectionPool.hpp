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
#include "../thread/Thread.hpp"

class DbConnection;

class DbConnectionPool : public Shareable<DbConnectionPool>, public Named
{
protected:
	mutable Log _log;
	using mutex_t = std::mutex;
	mutex_t _mutex;

	std::map<Thread::Id, std::shared_ptr<DbConnection>> _captured;
	std::deque<std::shared_ptr<DbConnection>> _pool;

	virtual std::shared_ptr<DbConnection> create() = 0;

public:
	DbConnectionPool() = delete;
	DbConnectionPool(const DbConnectionPool&) = delete;
	DbConnectionPool& operator=(const DbConnectionPool&) = delete;
	DbConnectionPool(DbConnectionPool&&) noexcept = delete;
	DbConnectionPool& operator=(DbConnectionPool&&) noexcept = delete;

	explicit DbConnectionPool(const Setting& setting);
	~DbConnectionPool() override = default;

	Log& log() const
	{
		return _log;
	}

	std::shared_ptr<Metric> metricSumConnections;
	std::shared_ptr<Metric> metricCurrenConnections;
	std::shared_ptr<Metric> metricSuccessQueryCount;
	std::shared_ptr<Metric> metricFailQueryCount;
	std::shared_ptr<Metric> metricAvgQueryPerSec;
	std::shared_ptr<Metric> metricAvgExecutionTime;

	void touch();

	std::shared_ptr<DbConnection> captureDbConnection();

	void releaseDbConnection(const std::shared_ptr<DbConnection>& conn);

	virtual void close() = 0;

	void attachDbConnection(const std::shared_ptr<DbConnection>& conn);

	std::shared_ptr<DbConnection> detachDbConnection();
};

#include "DbConnectionPoolFactory.hpp"

#define REGISTER_DBCONNECTIONPOOL(Type,Class) const Dummy Class::__dummy = \
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
    Class& operator=(const Class&) = delete;                                                    \
	Class(Class&&) noexcept = delete;                                                           \
	Class& operator=(Class&&) noexcept = delete;                                                \
    ~Class() override = default;                                                                \
                                                                                                \
private:                                                                                        \
    explicit Class(const Setting& setting);                                                     \
																								\
public:                                                                                         \
	void close() override;                                                                      \
                                                                                                \
private:                                                                                        \
	static const Dummy __dummy;
