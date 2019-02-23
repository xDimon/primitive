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
// File created on: 2017.02.26

// ThreadPool.cpp


#include <ucontext.h>
#include "ThreadPool.hpp"
#include "../utils/Time.hpp"
#include "../utils/Daemon.hpp"
#include "TaskManager.hpp"

// the constructor just launches some amount of _workers
ThreadPool::ThreadPool()
: _log("ThreadPool")
, _lastWorkerId(0)
{
}

void ThreadPool::hold()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	++pool._hold;
	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::unhold()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	--pool._hold;
}

void ThreadPool::setThreadNum(size_t num)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._workerMutex);

	size_t remain = (num < pool._workers.size()) ? 0 : (num - pool._workers.size());
	while (remain-- > 0)
	{
		pool.createThread();
	}

	pool._workersWakeupCondition.notify_one();
}

size_t ThreadPool::genThreadId()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	return ++pool._lastWorkerId;
}

void ThreadPool::createThread()
{
	std::function<void()> threadLoop =
	[this]
	{
		Task::Time waitUntil = std::chrono::steady_clock::now() + std::chrono::seconds(1);

		auto continueCondition =
			[this,&waitUntil]
			{
				std::lock_guard<std::mutex> lockGuard(_counterMutex);
				if (Daemon::shutingdown())
				{
					return true;
				}
				if (TaskManager::empty())
				{
					return _hold == 0;
				}
				waitUntil = TaskManager::waitUntil();

				return waitUntil <= std::chrono::steady_clock::now();
			};

		_log.debug("Begin thread's loop");

		while (!TaskManager::empty() || _hold)
		{
			{
				std::unique_lock<std::mutex> lock(_workerMutex);

				// Condition for run thread
				if (!_workersWakeupCondition.wait_until(lock, waitUntil, continueCondition))
				{
					continue;
				}
			}

			// Execute task
			TaskManager::executeOne();

			std::lock_guard<std::mutex> lockGuard(_contextsMutex);

			if (!_readyForContinueContexts.empty() && Thread::getCurrContextCount() > Thread::sizeContextForReplace())
			{
				auto context = _readyForContinueContexts.front();
				_readyForContinueContexts.pop();

				Thread::putContextForReplace(context);

//				_log.info("End continueContext => %p", context);
			}

			if (Thread::sizeContextForReplace() > 0)
			{
//				_log.info("End of secondary task");

				_workersWakeupCondition.notify_one();

				break;
			}

			_workersWakeupCondition.notify_one();
		}

		_log.debug("End thread's loop");
	};

	_log.debug("Thread create");

	auto thread = new Thread(threadLoop);

	thread->waitStart();

	_workers.emplace(thread->id(), thread);

	_workersWakeupCondition.notify_one();
}

void ThreadPool::wait()
{
	auto& pool = getInstance();

	pool._log.debug("Waiting for threadpool close");

	// Condition for close pool
	for(;;)
	{
		// Wait end all threads
		{
			std::lock_guard<std::mutex> lockGuard(pool._workerMutex);
			for (auto i = pool._workers.begin(); i != pool._workers.end(); )
			{
				auto ci = i++;
				Thread *thread = ci->second;
				if (thread->finished())
				{
					pool._workers.erase(ci);
					delete thread;
				}
			}
			if (pool._workers.empty())
			{
				break;
			}
			if (Daemon::shutingdown())
			{
				pool._workersWakeupCondition.notify_all();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

Thread *ThreadPool::getThread(Thread::Id tid)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._workerMutex);

	auto i = pool._workers.find(tid);
	if (i == pool._workers.end())
	{
		throw std::runtime_error("Unrecognized thread");
	}
	return i->second;
}

bool ThreadPool::empty()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._workerMutex);

	return pool._workers.empty();
}

void ThreadPool::continueContext(ucontext_t* context)
{
	if (!context)
	{
		return;
	}

	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._contextsMutex);

//	pool._log.info("Begin continueContext <= %p", context);

	pool._readyForContinueContexts.emplace(context);
}
