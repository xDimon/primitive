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

TaskManager::TaskManager()
: _log("TaskManager")//, Log::Detail::TRACE)
{
}

void TaskManager::enqueue(Task::Func&& func, Task::Time time, const char* label)
{
	auto& instance = getInstance();

	std::lock_guard<std::recursive_mutex> lockGuard(instance._mutex);

	instance._queue.emplace(std::forward<Task::Func>(func), time, label);

	instance._log.trace("Task queue length increase to %zu: %s", instance._queue.size(), label);

	ThreadPool::wakeup();
}

Task::Time TaskManager::waitUntil()
{
	auto& instance = getInstance();

	std::lock_guard<std::recursive_mutex> lockGuard(instance._mutex);

	return
		instance._queue.empty()
		? Task::Clock::now() + std::chrono::seconds(1)
		: instance._queue.top().until();
}

void TaskManager::executeOne()
{
	auto& instance = getInstance();

	instance._mutex.lock();

	again:

	if (instance._queue.empty())
	{
		instance._log.trace("Empty task queue");
		instance._mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		return;
	}

	auto u = std::chrono::duration_cast<std::chrono::microseconds>(instance._queue.top().until().time_since_epoch()).count();
	auto n = std::chrono::duration_cast<std::chrono::microseconds>(Task::Clock::now().time_since_epoch()).count();

//	if (instance._queue.top().until() > Task::Clock::now() && !Daemon::shutingdown())
	if (u > n && !Daemon::shutingdown())
	{
//		instance._log.trace("Reenqueue task (wait for %lld µs)", std::chrono::duration_cast<std::chrono::microseconds>(u - n).count());
		instance._log.trace("Reenqueue task (wait for %lld µs): %s", u - n, instance._queue.top().label());
		instance._mutex.unlock();
		return;
	}

	if (instance._queue.top().isDummy())
	{
		instance._queue.pop();
		goto again;
	}

	Task task{const_cast<Task&&>(instance._queue.top())};
	instance._queue.pop();

	instance._log.trace("Task queue length decrease to %zu: %s", instance._queue.size(), task.label());

	auto w = instance._queue.size();
//	instance._log.trace("Execute task (%zu waits) (late %lld µs)", w, std::chrono::duration_cast<std::chrono::microseconds>(n - u).count());
	instance._log.trace("Execute task (%zu waits) (late %lld µs)", w, n - u);
	if (n - u > 999999999)
	{
		instance._log.trace("(%lld, %lld)", n, u);
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

	std::lock_guard<std::recursive_mutex> lockGuard(instance._mutex);

	return instance._queue.empty();
}

size_t TaskManager::queueSize()
{
	auto& instance = getInstance();

	std::lock_guard<std::recursive_mutex> lockGuard(instance._mutex);

	return instance._queue.size();
}
