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
// File created on: 2017.05.23

// Task.cpp


#include <ucontext.h>
#include "Task.hpp"
#include "../log/Log.hpp"
#include "ThreadPool.hpp"
#include "../utils/Daemon.hpp"
#include "RollbackStackAndRestoreContext.hpp"

Task::Task(const std::shared_ptr<Func>& function)
: _function(function)
, _until(Time::min())
, _ctxId(0)
{
}

Task::Task(const std::shared_ptr<Func>& function, Duration delay)
: _function(function)
, _until(Clock::now() + delay)
, _ctxId(0)
{
}

Task::Task(const std::shared_ptr<Func>& function, Time time)
: _function(function)
, _until(time)
, _ctxId(0)
{
}

Task::Task(Task &&that) noexcept
: _function(std::move(that._function))
, _until(that._until)
, _ctxId(that._ctxId)
{
	that._until = Time::min();
	that._ctxId = 0;
}

Task& Task::operator=(Task &&that) noexcept
{
	_function = std::move(that._function);
	_until = that._until;
	that._until = Time::min();
	_ctxId = that._ctxId;
	that._ctxId = 0;
	return *this;
}

bool Task::operator()()
{
	if (_until != Time::min() && Clock::now() < _until && !Daemon::shutingdown())
	{
		return false;
	}

	(*_function)();

	return true;
}

bool Task::operator<(const Task& that) const
{
	return this->_until > that._until;
}

void Task::saveCtx(uint64_t ctxId)
{
	if (_ctxId != 0)
	{
		throw std::runtime_error("Context Id already set");
	}
	_ctxId = ctxId;
}

void Task::restoreCtx()
{
	if (_ctxId == 0)
	{
		return;
		throw std::runtime_error("Context Id didn't set");
	}
	ThreadPool::continueContext(_ctxId);
	_ctxId = 0;
}
