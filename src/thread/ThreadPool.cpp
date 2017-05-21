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
	_condition.notify_all();

	// Wait end all threads
	for (auto i : _workers)
	{
		i.second->join();
	}
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
}

size_t ThreadPool::genThreadId()
{
	std::unique_lock<std::mutex> lock(getInstance()._counterMutex);
	return ++getInstance()._workerCounter;
}

void ThreadPool::createThread()
{
	log().debug("Create new thread");

	auto thread = new Thread([this](){
		std::function<void()> task;

		Log log("ThreadLoop");
		for (;;)
		{
			log.trace("Wait task on thread");

			// Try to get or wait task
			{
				std::unique_lock<std::mutex> lock(_queueMutex);

				// Condition for run thread
				_condition.wait(lock, [this](){
					return _workerNumber == 0 || !_tasks.empty();
				});

				// Condition for end thread
				if (_workerNumber == 0 && _tasks.empty())
				{
					log.debug("End thread's loop");
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
			log.debug("Begin execution task on thread");
			task();
			log.debug("End execution task on thread");
		}
	});

	_workers.emplace(thread->id(), thread);
	//_workers.insert(std::make_pair(thread->id(), thread));
}

void ThreadPool::wait()
{
	getInstance().log().debug("Wait threadpool close");

	std::unique_lock<std::mutex> lock(getInstance()._queueMutex);

	// Condition for close pool
	getInstance()._condition.wait(
		lock, [pool = &getInstance()]()->bool {
			return !pool->_workerNumber && pool->_tasks.empty();
		}
	);
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

	// don't allow enqueueing after stopping the pool
	if (getInstance()._workerNumber == 0)
	{
		throw std::runtime_error("enqueue on stopped ThreadPool");
	}

	getInstance()._tasks.emplace(task);

	getInstance()._condition.notify_all();
}
