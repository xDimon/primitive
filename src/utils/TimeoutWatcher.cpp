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
// File created on: 2017.07.20

// TimeoutWatcher.cpp


#include "TimeoutWatcher.hpp"
#include "Timeout.hpp"
#include "../thread/ThreadPool.hpp"
#include "../thread/TaskManager.hpp"

TimeoutWatcher::TimeoutWatcher(const std::shared_ptr<Timeout>& timeout)
: _wp(timeout)
, _refCounter(0)
{
};

void TimeoutWatcher::operator()()
{
	auto timeout = _wp.lock();
	if (!timeout)
	{
		return;
	}

	timeout->_mutex.lock();

	_refCounter--;

	if ((*timeout)())
	{
		return;
	}

	if (_refCounter == 0)
	{
		restart(timeout->realExpireTime);
	}

	timeout->_mutex.unlock();
}

void TimeoutWatcher::restart(std::chrono::steady_clock::time_point time)
{
	auto timeout = _wp.lock();
	if (!timeout)
	{
		return;
	}

	++_refCounter;

	TaskManager::enqueue(
		[p = ptr()]
		{
			(*p)();
		},
		time
	);
}
