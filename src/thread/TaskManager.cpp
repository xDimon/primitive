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
// File created on: 2017.12.10

// TaskManager.cpp


#include "TaskManager.hpp"
#include "../utils/Daemon.hpp"
#include "ThreadPool.hpp"
#include "RollbackStackAndRestoreContext.hpp"

#if __cplusplus < 201703L
#define constexpr
#endif

TaskManager::TaskManager()
: _log("TaskManager")//, Log::Detail::TRACE)
{
}

void TaskManager::enqueue(Task::Func&& func, Task::Time time, const char* label)
{
	auto& instance = getInstance();

	{
		std::lock_guard<mutex_t> lockGuard(instance._mutex);

		instance._queue.emplace(std::forward<Task::Func>(func), time, label);

		if constexpr (std::is_same<mutex_t, std::recursive_mutex>::value)
		{
			auto n = std::chrono::duration_cast<std::chrono::microseconds>(Task::Clock::now().time_since_epoch()).count();
			auto u = std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count();

			instance._log.trace("Task queue length increase to %zu: %s (after %lld)", instance._queue.size(), label, u - n);

			u = std::chrono::duration_cast<std::chrono::microseconds>(instance._queue.top().until().time_since_epoch()).count();

			instance._log.trace(" Next task after %lld µs: %s", u - n, instance._queue.top().label());
		}
	}

	ThreadPool::wakeup();
}

Task::Time TaskManager::waitUntil()
{
	auto& instance = getInstance();

	std::lock_guard<mutex_t> lockGuard(instance._mutex);

	return
		instance._queue.empty()
		? Task::Clock::now() + std::chrono::seconds(1)
		: instance._queue.top().until();
}

void TaskManager::executeOne()
{
	auto& instance = getInstance();

	instance._mutex.lock();

	if (instance._queue.empty())
	{
		if constexpr (std::is_same<mutex_t, std::recursive_mutex>::value)
		{
			instance._log.trace("Empty task queue");
		}

		instance._mutex.unlock();
		return;
	}

	if constexpr (std::is_same<mutex_t, std::recursive_mutex>::value)
	{
		auto u = std::chrono::duration_cast<std::chrono::microseconds>(instance._queue.top().until().time_since_epoch()).count();
		auto n = std::chrono::duration_cast<std::chrono::microseconds>(Task::Clock::now().time_since_epoch()).count();

		if (u > n && !Daemon::shutingdown())
		{
			instance._log.trace("Reenqueue task (wait for %lld µs): %s", u - n, instance._queue.top().label());
			instance._mutex.unlock();
			return;
		}
	}
	else
	{
		if (instance._queue.top().until() > Task::Clock::now() && !Daemon::shutingdown())
		{
			instance._mutex.unlock();
			return;
		}
	}

	Task task(std::move(const_cast<Task&>(instance._queue.top())));

	while (!instance._queue.empty() && instance._queue.top().isDummy())
	{
		if constexpr (std::is_same<mutex_t, std::recursive_mutex>::value)
		{
			auto label = instance._queue.top().label();
			instance._queue.pop();

			instance._log.trace("Task queue length decrease to %zu: %s", instance._queue.size(), label);
		}
		else
		{
			instance._queue.pop();
		}
	}

	if constexpr (std::is_same<mutex_t, std::recursive_mutex>::value)
	{
		auto n = std::chrono::duration_cast<std::chrono::microseconds>(Task::Clock::now().time_since_epoch()).count();
		auto u = std::chrono::duration_cast<std::chrono::microseconds>(task.until().time_since_epoch()).count();

		if (!instance._queue.empty())
		{
			auto u = std::chrono::duration_cast<std::chrono::microseconds>(instance._queue.top().until().time_since_epoch()).count();

			instance._log.trace(" Next task after %lld µs: %s", u - n, instance._queue.top().label());
		}
		else
		{
			instance._log.trace(" No next task");
		}

		auto w = instance._queue.size();
		instance._log.trace("Execute task (%zu waits) (late %lld µs)", w, n - u);
	}

	instance._mutex.unlock();

	ThreadPool::wakeup();

	try
	{
		task.execute();
	}
	catch (const RollbackStackAndRestoreContext& exception)
	{
//		throw;
	}
	catch (const std::exception& exception)
	{
		instance._log.warn("Uncatched exception at execute task of pool: %s", exception.what());
	}
}

bool TaskManager::empty()
{
	auto& instance = getInstance();

	std::lock_guard<mutex_t> lockGuard(instance._mutex);

	return instance._queue.empty();
}

size_t TaskManager::queueSize()
{
	auto& instance = getInstance();

	std::lock_guard<mutex_t> lockGuard(instance._mutex);

	return instance._queue.size();
}
