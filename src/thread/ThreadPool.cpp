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
// File created on: 2017.02.26

// ThreadPool.cpp


#include "ThreadPool.hpp"
#include "../utils/ShutdownManager.hpp"
#include "../utils/Time.hpp"

// the constructor just launches some amount of _workers
ThreadPool::ThreadPool()
: _log("ThreadPool")
, _workerCounter(0)
, _workerNumber(0)
{
}

// the destructor joins all threads
ThreadPool::~ThreadPool()
{
	// Wait end all threads
	while (!_workers.empty())
	{
		auto i = _workers.begin();
		i->second->join();
		_workers.erase(i);
		delete i->second;
	}
}

void ThreadPool::hold()
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._counterMutex);
	++pool._hold;
	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::unhold()
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._counterMutex);
	--pool._hold;
}

void ThreadPool::setThreadNum(size_t num)
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._queueMutex);

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

	std::unique_lock<std::mutex> lock(pool._counterMutex);
	return ++pool._workerCounter;
}

void ThreadPool::createThread()
{
	std::function<void()> threadLoop = [this]()
	{
		Task::Time waitUntil = std::chrono::steady_clock::now() + std::chrono::hours(24);
//								std::chrono::time_point<std::chrono::steady_clock>::max();

		auto continueCondition = [this,&waitUntil](){
			std::unique_lock<std::mutex> lock(_counterMutex);
			if (ShutdownManager::shutingdown())
			{
				return true;
			}
			if (_tasks.empty())
			{
				return _hold == 0;
			}
			waitUntil = _tasks.top()->until();
			return waitUntil <= std::chrono::steady_clock::now();
		};

		_log.debug("Begin thread's loop");

		std::shared_ptr<Task> task;

		for (;;)
		{
			task.reset();

			{
				std::unique_lock<std::mutex> lock(_queueMutex);

//				auto now = std::chrono::steady_clock::now();
//				if (!_tasks.empty())
//				{
//					_log.trace("Wait: task =%20lld", _tasks.top()->until());
//				}
//				_log.trace("Wait: until=%20lld", waitUntil);
//				_log.trace("Wait:   now=%20lld", now);
//				_log.trace("Wait for    %20lld", waitUntil - now);
//				_log.trace("Queue       %s", _tasks.empty()?"empty":"have a task");
//				_log.trace("Hold        %s", _hold?"yes":"no");

				// Condition for run thread
				if (!_workersWakeupCondition.wait_until(lock, waitUntil, continueCondition))
				{
					waitUntil = std::chrono::steady_clock::now() + std::chrono::hours(24);
//								std::chrono::time_point<std::chrono::steady_clock>::max();
					continue;
				}

				// Condition for end thread
				if (_tasks.empty())
				{
					_log.debug("End thread's loop");
					break;
				}

				_log.trace("Get task from queue");

				// Get task from queue
				task = _tasks.top();
				_tasks.pop();

				// Condition for end thread
				if (!_tasks.empty())
				{
					_workersWakeupCondition.notify_one();
				}
			}

			// Execute task
			_log.trace("Begin execution task on thread");

			bool done = true;
			try
			{
				done = (*task)();
			}
			catch (const std::exception& exception)
			{
				_log.warn("Uncatched exception at execute task of pool: %s", exception.what());
			}
			if (!done)
			{
				_log.trace("Reenqueue task");

				std::unique_lock<std::mutex> lock(_queueMutex);
				waitUntil = task->until();
				_tasks.emplace(std::move(task));
			}
			else
			{
				_log.trace("End execution task on thread");
//				waitUntil = std::chrono::steady_clock::now();
//				std::chrono::time_point<std::chrono::steady_clock>::min();
			}

			_log.trace("Waiting for task on thread");
		}

//		{
//			std::unique_lock<std::mutex> lock(pool._queueMutex);
//			auto i = _workers.find(std::this_thread::get_id());
//			_workers.erase(i);
//			delete i->second;
//		}

		_workersWakeupCondition.notify_one();
	};

	_log.debug("Thread create");

	auto thread = new Thread(threadLoop);

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
			std::unique_lock<std::mutex> lock(pool._queueMutex);
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
				if (ShutdownManager::shutingdown())
				{
					pool._workersWakeupCondition.notify_all();
				}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

Thread *ThreadPool::getCurrent()
{
	auto& pool = getInstance();

	auto i = pool._workers.find(std::this_thread::get_id());
	if (i == pool._workers.end())
	{
		throw std::runtime_error("Unrecognized thread");
	}
	return i->second;
}

void ThreadPool::enqueue(const std::shared_ptr<Task>& task)
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._queueMutex);

	pool._tasks.emplace(task);

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function)
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function));

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function, Task::Duration delay)
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function, delay));

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function, Task::Time time)
{
	auto& pool = getInstance();

	std::unique_lock<std::mutex> lock(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function, time));

	pool._workersWakeupCondition.notify_one();
}
