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


#include <iostream>
#include "ThreadPool.hpp"

// the constructor just launches some amount of _workers
ThreadPool::ThreadPool()
: Log("ThreadPool")
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
	_poolCloseCondition.notify_one();

	// Wait end all threads
	for (auto i : _workers)
	{
		i.second->join();
	}
}

void ThreadPool::hold()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	++getInstance()._hold;
//	getInstance().log().debug("Hold to %d", getInstance()._hold);
	getInstance()._workersWakeupCondition.notify_one();
	getInstance()._poolCloseCondition.notify_one();
}

void ThreadPool::unhold()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	--getInstance()._hold;
//	getInstance().log().debug("Unhold to %d", getInstance()._hold);
	getInstance()._poolCloseCondition.notify_one();
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
		std::function<void()> task;

		auto continueCondition = [this](){
			return !_tasks.empty() || (_tasks.empty() && !_hold);
		};

		Log log("ThreadLoop");

		log.debug("Begin thread's loop");

		for (;;)
		{
//			log.trace("Wait task on thread");

			// Try to get or wait task
			{
				std::unique_lock<std::mutex> lock(_queueMutex);

				// Condition for run thread
				_workersWakeupCondition.wait(lock, continueCondition);

				// Condition for end thread
				if (_tasks.empty())
				{
					log.debug("End thread's loop");
					_workersWakeupCondition.notify_one();
					break;
				}

				log.trace("Get task from queue");

				// Get task from queue
				task = std::move(_tasks.front());
				_tasks.pop();
			}

			log.debug("Execute task on thread");
//			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// Execute task
//			log.debug("Begin execution task on thread");
			task();
//			log.debug("End execution task on thread");
		}

		std::unique_lock<std::mutex> lock(getInstance()._queueMutex);
		auto i = _workers.find(std::this_thread::get_id());
		_workers.erase(i);
	};

	log().debug("Thread create");

	auto thread = new Thread(threadLoop);

	_workers.emplace(thread->id(), thread);
}

void ThreadPool::wait()
{
	getInstance().log().debug("Wait threadpool close");

	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	auto& pool = getInstance();

	auto continueCondition = [&pool]()->bool {
		return pool._hold == 0 && pool._tasks.empty() && pool._workers.empty();
	};

	// Condition for close pool
	getInstance()._workersWakeupCondition.wait(lock, continueCondition);
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

void ThreadPool::enqueue(std::function<void()> task)
{
	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

//	// don't allow enqueueing after stopping the pool
//	if (getInstance()._workerNumber == 0)
//	{
//		throw std::runtime_error("enqueue on stopped ThreadPool");
//	}

	getInstance()._tasks.emplace(task);

	getInstance()._workersWakeupCondition.notify_one();
}
