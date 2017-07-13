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

Task::Task()
: _function()
, _until(Time::min())
, _immediately(true)
, _tmpContext(nullptr)
, _mainContext(nullptr)
{
}

Task::Task(Func function)
: _function(std::move(function))
, _until(Time::min())
, _immediately(true)
, _tmpContext(nullptr)
, _mainContext(nullptr)
{
}

Task::Task(Func function, Duration delay)
: _function(std::move(function))
, _until(Clock::now() + delay)
, _immediately(false)
, _tmpContext(nullptr)
, _mainContext(nullptr)
{
}

Task::Task(Func function, Time time)
: _function(std::move(function))
, _until(time)
, _immediately(false)
, _tmpContext(nullptr)
, _mainContext(nullptr)
{
}

Task::Task(Task const &&that)
: _function(std::move(that._function))
, _until(std::move(that._until))
, _immediately(that._immediately)
, _tmpContext(that._tmpContext)
, _mainContext(that._mainContext)
{
}

void Task::operator=(Task const &&that)
{
	_function = std::move(that._function);
	_until = std::move(that._until);
	_immediately = that._immediately;
	_tmpContext = that._tmpContext;
	_mainContext = that._mainContext;
}

bool Task::operator()()
{
	if (!_immediately && Clock::now() < _until && !ThreadPool::stops())
	{
		return false;
	}

	_function();

	return true;
}

bool Task::operator<(const Task& that) const
{
	if (this->_immediately)
	{
		return false;
	}
	else if (that._immediately)
	{
		return true;
	}
	else
	{
		return this->_until > that._until;
	}
}

void Task::saveContext(ucontext_t* tmpContext, ucontext_t* mainContext)
{
	_tmpContext = tmpContext;
	_mainContext = mainContext;

}

void Task::restoreContext() const
{
	if (!_tmpContext || !_mainContext)
	{
		return;
	}

	// при завершении таска вернуться в связанный контекст, и удалить предыдущий
	if (swapcontext(_tmpContext, _mainContext))
	{
		throw std::runtime_error("Can't change context");
	}
}
