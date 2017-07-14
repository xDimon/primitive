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
	// Switch to stop
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		_workerNumber = 0;
	}

	// Talk to stop threads
	_workersWakeupCondition.notify_all();

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
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	++getInstance()._hold;
	getInstance()._workersWakeupCondition.notify_one();
}

void ThreadPool::unhold()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	--getInstance()._hold;
}

bool ThreadPool::stops()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	return !getInstance()._workerNumber;
}

void ThreadPool::setThreadNum(size_t num)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);
	getInstance()._workerNumber = num;

	size_t remain = (num < getInstance()._workers.size()) ? 0 : (num - getInstance()._workers.size());
	while (remain--)
	{
		getInstance().createThread();
	}

	getInstance()._workersWakeupCondition.notify_one();
}

size_t ThreadPool::genThreadId()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	return ++getInstance()._workerCounter;
}

void ThreadPool::createThread()
{
	std::function<void()> threadLoop = [this]()
	{
		Task::Time waitUntil = std::chrono::steady_clock::now() + std::chrono::hours(24);
//								std::chrono::time_point<std::chrono::steady_clock>::max();

		auto continueCondition = [this,&waitUntil](){
			std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
			if (_tasks.empty())
			{
				return !_hold;
			}
			else if (!_workerNumber)
			{
				return true;
			}
			else
			{
				waitUntil = _tasks.top().until();
				return waitUntil <= std::chrono::steady_clock::now();
			}
		};

		Log log("ThreadLoop");

		log.debug("Begin thread's loop");

		for (;;)
		{
			Task task;

			log.trace("Wait task on thread");

			{
				std::unique_lock<std::mutex> lock(_queueMutex);

//				auto now = std::chrono::steady_clock::now();
//				if (!_tasks.empty()) log.trace("Wait: task =%20lld", _tasks.top().until());
//				log.trace("Wait: until=%20lld", waitUntil);
//				log.trace("Wait:   now=%20lld", now);
//				log.trace("Wait for    %20lld", waitUntil - now);
//				log.trace("Queue       %s", _tasks.empty()?"empty":"have a task");
//				log.trace("Hold        %s", _hold?"yes":"no");

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
					log.debug("End thread's loop");
					break;
				}

				log.trace("Get task from queue");

				// Get task from queue
				task = std::move(_tasks.top());
				_tasks.pop();

				// Condition for end thread
				if (!_tasks.empty())
				{
					_workersWakeupCondition.notify_one();
				}
			}

			// Execute task
			log.debug("Begin execution task on thread");
			bool done = task();
			if (!done)
			{
				std::unique_lock<std::mutex> lock(_queueMutex);
				waitUntil = task.until();
				_tasks.emplace(std::move(task));
			}
			else
			{
//				waitUntil = std::chrono::steady_clock::now();
//				std::chrono::time_point<std::chrono::steady_clock>::min();
			}
			log.debug("End execution task on thread");
		}

//		{
//			std::unique_lock<std::mutex> lock(getInstance()._queueMutex);
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
	getInstance()._log.debug("Wait threadpool close");

	auto& pool = getInstance();

	// Condition for close pool
	for(;;)
	{
		// Wait end all threads
		{
			std::unique_lock<std::mutex> qLock(pool._queueMutex);
			for (auto i = pool._workers.begin(); i != pool._workers.end(); )
			{
				auto ci = i++;
				Thread *thread = ci->second;
				if (thread->finished())
				{
					pool._workers.erase(ci);
					thread->join();
					delete thread;
				}
			}
			std::unique_lock<std::mutex> cLock(getInstance()._counterMutex);
			if (!pool._hold)
			{
				if (pool._workers.empty())
				{
					break;
				}
				if (!pool._workerNumber)
				{
					pool._workersWakeupCondition.notify_all();
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

Thread *ThreadPool::getCurrent()
{
	auto i = getInstance()._workers.find(std::this_thread::get_id());
	if (i == getInstance()._workers.end())
	{
		throw std::runtime_error("Unrecognized thread");
	}
	return i->second;
}

void ThreadPool::enqueue(Task& task)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	getInstance()._tasks.emplace(std::move(task));

//	getInstance()._log.trace("  Wake up One worker (%d)", __LINE__);
	getInstance()._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(Task&& task)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	getInstance()._tasks.emplace(std::move(task));

	getInstance()._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(Task::Func function)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	getInstance()._tasks.emplace(std::move(function));

	getInstance()._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(Task::Func function, Task::Duration delay)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	getInstance()._tasks.emplace(std::move(function), delay);

	getInstance()._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(Task::Func function, Task::Time time)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	getInstance()._tasks.emplace(std::move(function), time);

	getInstance()._workersWakeupCondition.notify_one();
}
