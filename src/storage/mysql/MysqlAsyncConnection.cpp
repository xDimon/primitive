// Copyright © 2019 Dmitriy Khaustov
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
// File created on: 2019.03.16

// MysqlAsyncConnection.cpp


#include "MysqlAsyncConnection.hpp"

#ifdef MARIADB_BASE_VERSION // https://mariadb.com/kb/en/library/using-the-non-blocking-library/

#include <cstring>
#include "MysqlConnectionPool.hpp"
#include "MysqlResult.hpp"
#include "../../thread/Thread.hpp"
#include "../../net/ConnectionManager.hpp"
#include "../../thread/TaskManager.hpp"
#include "../../thread/RollbackStackAndRestoreContext.hpp"

void MysqlAsyncConnection::implWait(bool isNew)
{
	auto pool = _pool.lock();
	if (pool)
	{
		if (isNew)
		{
			pool->log().warn("Enter implWait #%zu (new)", id);

			auto holder = std::make_shared<Task::Func>(
				[iam = std::dynamic_pointer_cast<MysqlAsyncConnection>(ptr())]
				{
					iam->_ctx = Thread::getCurrTaskContext();
					Thread::setCurrTaskContext(nullptr);
					ConnectionManager::watch(iam->_helper);
				}
			);
//			auto& alarm = *holder;
//
//			// Выполняем асинхронно
//			Thread::self()->yield(alarm);

			pool->log().warn("Return implWait #%zu (new)", id);
		}
		else
		{
			pool->log().warn("Enter implWait #%zu (old)", id);
			pool->detachDbConnection();

			auto holder = std::make_shared<Task::Func>(
				[this]
				{
					_ctx = Thread::getCurrTaskContext();
					Thread::setCurrTaskContext(nullptr);
					ConnectionManager::watch(_helper);
				}
			);
//			auto& alarm = *holder;
//
//			// Выполняем асинхронно
//			Thread::self()->yield(alarm);

			pool->attachDbConnection(DbConnection::ptr());
			pool->log().warn("Return implWait #%zu (old)", id);
		}
	}
}

bool MysqlAsyncConnection::implQuery(const std::string& sql)
{
	int ret = 0;
	auto pool = _pool.lock();

	if (pool) pool->log().warn("Enter    QUERY #%zu", id);
	_status = mysql_real_query_start(&ret, _mysql, sql.c_str(), sql.length());
	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone QUERY #%zu", id);
		implWait();
		_status = mysql_real_query_cont(&ret, _mysql, _status);
	}
	if (pool) pool->log().warn("Return   QUERY #%zu", id);

	return ret == 0;
}

MYSQL_RES* MysqlAsyncConnection::implStoreResult()
{
	MYSQL_RES* ret;
	auto pool = _pool.lock();

	if (pool) pool->log().warn("Enter    STORE #%zu", id);
	_status = mysql_store_result_start(&ret, _mysql);
	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone STORE #%zu", id);
		implWait();
		_status = mysql_store_result_cont(&ret, _mysql, _status);
	}
	if (pool) pool->log().warn("Return   STORE #%zu", id);

	return ret;
}

int MysqlAsyncConnection::implNextResult()
{
	int ret;
	auto pool = _pool.lock();

	if (pool) pool->log().warn("Enter    NEXT #%zu", id);
	_status = mysql_next_result_start(&ret, _mysql);
	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone NEXT #%zu", id);
		implWait();
		_status = mysql_next_result_cont(&ret, _mysql, _status);
	}
	if (pool) pool->log().warn("Return   NEXT #%zu", id);

	return ret;
}

void MysqlAsyncConnection::implFreeResult(MYSQL_RES* result)
{
	auto pool = _pool.lock();

	if (pool) pool->log().warn("Enter    FREE #%zu", id);
	_status = mysql_free_result_start(result);
	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone FREE #%zu", id);
		implWait();
		_status = mysql_free_result_cont(result, _status);
	}
	if (pool) pool->log().warn("Return   FREE #%zu", id);
}

MysqlAsyncConnection::MysqlAsyncConnection(const std::shared_ptr<DbConnectionPool>& pool)
: DbConnection(std::dynamic_pointer_cast<MysqlConnectionPool>(pool))
, _mysql(nullptr)
, _transaction(0)
, _status(0)
, _ctx(nullptr)
{
	_mysql = mysql_init(_mysql);
	if (_mysql == nullptr)
	{
		throw std::runtime_error("Can't init mysql connection");
	}
}

MysqlAsyncConnection::~MysqlAsyncConnection()
{
	auto pool = _pool.lock();
	if (pool) pool->log().warn("Enter    CLOSE #%zu", id);
	_status = mysql_close_start(_mysql);
	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone CLOSE #%zu", id);
		implWait();
		_status = mysql_close_cont(_mysql, _status);
	}
	if (pool) pool->log().warn("Return   CLOSE #%zu", id);

	if (_helper)
	{
		ConnectionManager::remove(_helper);
	}
}

bool MysqlAsyncConnection::connect(
	const std::string& dbname,
	const std::string& dbuser,
	const std::string& dbpass,
	const std::string& dbserver,
	unsigned int dbport,
	const std::string& dbsocket,
	const std::string& charset,
	const std::string& timezone
)
{
	auto pool = _pool.lock();

	static const my_bool on = true;
	mysql_options(_mysql, MYSQL_OPT_RECONNECT, &on);

	mysql_options(_mysql, MYSQL_OPT_NONBLOCK, nullptr);

	unsigned int timeout = 900;
	mysql_options(_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

	if (!charset.empty())
	{
		mysql_options(_mysql, MYSQL_SET_CHARSET_NAME, charset.c_str());
	}
	if (!timezone.empty())
	{
		mysql_options(_mysql, MYSQL_INIT_COMMAND, ("SET time_zone='" + timezone + "';\n").c_str());
	}

	MYSQL *ret = nullptr;

	if (pool) pool->log().warn("Enter    CONNECT #%zu", id);
	_status = mysql_real_connect_start(
		&ret,
		_mysql,
		dbserver.c_str(),
		dbuser.c_str(),
		dbpass.c_str(),
		dbname.c_str(),
		dbport,
		dbsocket.c_str(),
		CLIENT_REMEMBER_OPTIONS
	);

	_helper = std::make_shared<MysqlAsyncConnectionHelper>(std::dynamic_pointer_cast<MysqlAsyncConnection>(ptr()), mysql_get_socket(_mysql));

	ConnectionManager::add(_helper);

	while (_status != 0)
	{
		if (pool) pool->log().warn("Postpone CONNECT #%zu", id);
		implWait(true);
		_status = mysql_real_connect_cont(&ret, _mysql, _status);
	}
	if (pool) pool->log().warn("Return   CONNECT #%zu", id);

	if (ret == nullptr)
	{
		throw std::runtime_error(std::string("Can't connect to database ← ") + mysql_error(_mysql));
//		return false;
	}

	return true;
}

std::string MysqlAsyncConnection::escape(const std::string& str)
{
	size_t size = str.length() * 4 + 1;
	if (size > 1024)
	{
		auto buff = new char[size];
		mysql_escape_string(buff, str.c_str(), str.length());
		std::string result(buff);
		free(buff);
		return result;
	}
	else
	{
		char buff[size];
		mysql_escape_string(buff, str.c_str(), str.length());
		return std::forward<std::string>(buff);
	}
}

bool MysqlAsyncConnection::startTransaction()
{
	if (_transaction > 0)
	{
		_transaction++;
		return true;
	}

	if (DbConnection::query("START TRANSACTION;"))
	{
		_transaction = 1;
		return true;
	}

	return false;
}

bool MysqlAsyncConnection::deadlockDetected()
{
	return mysql_errno(_mysql) == 1213;
}

bool MysqlAsyncConnection::commit()
{
	if (_transaction > 1)
	{
		_transaction--;
		return true;
	}
	if (_transaction == 0)
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: commit when counter of transaction is zero");
		}
		return false;
	}
	if (DbConnection::query("COMMIT;"))
	{
		_transaction = 0;
		return true;
	}

	return false;
}

bool MysqlAsyncConnection::rollback()
{
	if (_transaction > 1)
	{
		_transaction--;
		return true;
	}
	if (_transaction == 0)
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: rollback when counter of transaction is zero");
		}
		return false;
	}
	if (!DbConnection::query("ROLLBACK;"))
	{
		if (auto pool = _pool.lock())
		{
			pool->log().warn("Internal error: Fail at rolling back");
		}
	}

	_transaction = 0;
	return true;
}

bool MysqlAsyncConnection::query(const std::string& sql, DbResult* res, size_t* affected, size_t* insertId)
{
	auto pool = _pool.lock();

	auto result = dynamic_cast<MysqlResult *>(res);

	auto beginTime = std::chrono::steady_clock::now();
	if (pool) pool->metricAvgQueryPerSec->addValue();

	if (pool) pool->log().debug("MySQL query: %s", sql.c_str());

	bool success = implQuery(sql);

	auto now = std::chrono::steady_clock::now();
	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(now - beginTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		if (pool) pool->metricAvgExecutionTime->addValue(timeSpent, now);
	}

	if (timeSpent >= 5)
	{
		if (pool) pool->log().warn("MySQL query too long: %0.03f sec\n\t\tFor query:\n\t\t%s", timeSpent, sql.c_str());
	}

	if (!success)
	{
		if (pool) pool->metricFailQueryCount->addValue();
		if (!deadlockDetected())
		{
			if (pool) pool->log().warn("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
		}
		return false;
	}

	if (pool) pool->log().debug("MySQL query success: " + sql);

	if (affected != nullptr)
	{
		*affected = mysql_affected_rows(_mysql);
	}

	if (insertId != nullptr)
	{
		*insertId = mysql_insert_id(_mysql);
	}

	if (result != nullptr)
	{
		result->set(implStoreResult());

		if (result->get() == nullptr && mysql_errno(_mysql) != 0)
		{
			if (pool) pool->metricFailQueryCount->addValue();
			if (pool) pool->log().error("MySQL store result error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());
			return false;
		}
	}

	if (pool) pool->metricSuccessQueryCount->addValue();
	return true;
}

bool MysqlAsyncConnection::multiQuery(const std::string& sql)
{
	auto pool = _pool.lock();

	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);

	auto beginTime = std::chrono::steady_clock::now();
	if (pool) pool->metricAvgQueryPerSec->addValue();

	bool success = implQuery(sql);

	auto now = std::chrono::steady_clock::now();
	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(now - beginTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		if (pool) pool->metricAvgExecutionTime->addValue(timeSpent, now);
	}

	if (!success)
	{
		if (pool) pool->log().error("MySQL query error: [%u] %s\n\t\tFor query:\n\t\t%s", mysql_errno(_mysql), mysql_error(_mysql), sql.c_str());

		mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
		if (pool) pool->metricFailQueryCount->addValue();
		return false;
	}

	for(;;)
	{
		MYSQL_RES* result = implStoreResult();
		if (result != nullptr)
		{
			implFreeResult(result);
		}
		if (implNextResult() != 0)
		{
			break;
		}
	}

	mysql_set_server_option(_mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	if (pool) pool->metricSuccessQueryCount->addValue();

	return true;
}

void MysqlAsyncConnection::MysqlAsyncConnectionHelper::watch(epoll_event& ev)
{
	auto parent = _parent.lock();
	if (!parent)
	{
		_log.trace("WATCH: No parent");
		return;
	}

	if (!parent->_mysql)
	{
		_log.trace("WATCH: No Mysql struct");
		return;
	}

	ev.data.ptr = this;
	ev.events = 0;

	ev.events |= EPOLLET; // Ждем появления НОВЫХ событий

	if (_closed)
	{
		return;
	}

	ev.events |= EPOLLERR;
	ev.events |= EPOLLRDHUP;
	ev.events |= EPOLLHUP;

	if (!_error)
	{
		if (parent->_status & MYSQL_WAIT_READ)
		{
			ev.events |= EPOLLIN;
		}
		if (parent->_status & MYSQL_WAIT_WRITE)
		{
			ev.events |= EPOLLOUT;
		}
		if (parent->_status & MYSQL_WAIT_EXCEPT)
		{
			ev.events |= EPOLLPRI;
		}
		if (parent->_status & MYSQL_WAIT_TIMEOUT)
		{
			setTtl(std::chrono::milliseconds(1000 * mysql_get_timeout_value(parent->_mysql)));
		}
		else
		{
			setTtl(std::chrono::seconds(900));
		}
	}
}

bool MysqlAsyncConnection::MysqlAsyncConnectionHelper::processing()
{
	auto parent = _parent.lock();
	if (!parent)
	{
		throw std::runtime_error("No parent for MysqlAsyncConnectionHelper");
	}

	parent->_status = 0;
	if (isReadyForRead()) parent->_status |= MYSQL_WAIT_READ;
	if (isReadyForWrite()) parent->_status |= MYSQL_WAIT_WRITE;
	if (wasFailure()) parent->_status |= MYSQL_WAIT_EXCEPT;
	if (timeIsOut()) parent->_status |= MYSQL_WAIT_TIMEOUT;

	throw RollbackStackAndRestoreContext(parent->_ctx);
}

#endif // MARIADB_BASE_VERSION
