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
// File created on: 2017.12.10

// STask.cpp


#include "STask.hpp"
#include "../utils/Daemon.hpp"
#include "TaskManager.hpp"
#include "ThreadPool.hpp"

STask::STask(Func&& function, Time until)
: _function(std::move(function))
, _until(until)
, _parentTaskContext(Thread::getContext())
{
}

STask::STask(STask &&that) noexcept
: _function(std::move(that._function))
, _until(that._until)
, _parentTaskContext(that._parentTaskContext)
{
	that._function = static_cast<void(*)()>(nullptr);
	that._until = Time::min();
	that._parentTaskContext = nullptr;
}

STask& STask::operator=(STask &&that) noexcept
{
	if (this == &that)
	{
		return *this;
	}

	_function = std::move(that._function);
	_until = that._until;
	_parentTaskContext = that._parentTaskContext;
	that._function = static_cast<void(*)()>(nullptr);
	that._until = Time::min();
	that._parentTaskContext = nullptr;

	return *this;
}

// Исполнение
void STask::execute()
{
	auto tmp = Thread::getCurrTaskContext();
	Thread::setCurrTaskContext(_parentTaskContext);

	std::exception_ptr currentException;
	try
	{
		if (_function)
		{
			_function();
		}
	}
	catch (const std::exception& exception)
	{
		currentException = std::current_exception();
	}

	_parentTaskContext = Thread::getCurrTaskContext();
	Thread::setCurrTaskContext(tmp);

	if (_parentTaskContext)
	{
		ThreadPool::continueContext(_parentTaskContext);
		_parentTaskContext = nullptr;
	}

	if (currentException)
	{
		std::rethrow_exception(currentException);
	}
}
