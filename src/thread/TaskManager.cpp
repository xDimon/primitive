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

// TaskManager.cpp


#include "TaskManager.hpp"
#include "../utils/Daemon.hpp"
#include "ThreadPool.hpp"
#include "RollbackStackAndRestoreContext.hpp"

TaskManager::TaskManager()
: _log("TaskManager")
{
}

void TaskManager::enqueue(STask::Func&& func, STask::Time time)
{
	auto& instance = getInstance();

	std::lock_guard<std::mutex> lockGuard(instance._mutex);

	instance._queue.emplace(std::forward<STask::Func>(func), time);

//	instance._log.info("Task queue length increase to %zu", instance._queue.size());

	ThreadPool::wakeup();
}

STask::Time TaskManager::waitUntil()
{
	auto& instance = getInstance();

	std::lock_guard<std::mutex> lockGuard(instance._mutex);

	return
		instance._queue.empty()
		? STask::Clock::now() + std::chrono::seconds(1)
		: instance._queue.top().until();
}

void TaskManager::executeOne()
{
	auto& instance = getInstance();

	instance._mutex.lock();

	if (instance._queue.empty())
	{
		instance._log.info("Empty task queue");
		instance._mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		return;
	}

	STask task{const_cast<STask&&>(instance._queue.top())};

	auto u = task.until();
	auto n = STask::Clock::now();

//	if (task.until() > STask::Clock::now() && !Daemon::shutingdown())
	if (u > n && !Daemon::shutingdown())
	{
		instance._log.info("Reenqueue task (wait for %lld µs)", std::chrono::duration_cast<std::chrono::microseconds>(u - n).count());
		instance._mutex.unlock();
		return;
	}

	instance._queue.pop();

//	instance._log.info("Task queue length decrease to %zu", instance._queue.size());

	instance._log.info("Execute task (%zu waits) (late %lld µs)", instance._queue.size(), std::chrono::duration_cast<std::chrono::microseconds>(n - u).count());

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

	std::lock_guard<std::mutex> lockGuard(instance._mutex);

	return instance._queue.empty();
}
