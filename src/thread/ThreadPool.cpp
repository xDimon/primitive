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
: _workerNum(0)
{
}

// the destructor joins all threads
ThreadPool::~ThreadPool()
{
	// Switch to stop
	{
		std::unique_lock<std::mutex> lock(_queue_mutex);
		_workerNum = 0;
	}

	// Talk to stop threads
	_condition.notify_all();

	// Wait end all threads
	for (std::thread &worker: _workers)
	{
		worker.join();
	}
}

void ThreadPool::setThreadNum(size_t num)
{
	{
		std::unique_lock<std::mutex> lock(getInstance()._queue_mutex);
		getInstance()._workerNum = num;
	}

	size_t remain = (num < getInstance()._workers.size()) ? 0 : (num - getInstance()._workers.size());
	while (remain--)
	{
		getInstance().createThread();
	}
}

void ThreadPool::createThread()
{
	_workers.emplace_back([this](){
		for (;;)
		{
			std::function<void()> task;

			// Try to get or wait task
			{
				std::cout << "Thread#" << std::this_thread::get_id() << ": Wait ThreadPool task" << std::endl;

				std::unique_lock<std::mutex> lock(_queue_mutex);

				// Condition for run thread
				this->_condition.wait(lock, [this](){
					return _workerNum == 0 || !_tasks.empty();
				});

				// Condition for end thread
				if (_workerNum == 0 && _tasks.empty())
				{
					return;
				}

				// Get task from queue
				task = std::move(_tasks.front());
				this->_tasks.pop();
			}

			std::cout << "Thread#" << std::this_thread::get_id() << ": Execute ThreadPool task" << std::endl;

			// Execute task
			task();
		}
	});
}

void ThreadPool::wait()
{
	for(;;)
	{
		std::cout << "Thread#" << std::this_thread::get_id() << ": Wait ThreadPool close" << std::endl;

		std::unique_lock<std::mutex> lock(getInstance()._queue_mutex);

		// Condition for close pool
		getInstance()._condition.wait(
			lock, [pool = &getInstance()]()->bool {
				return !pool->_workerNum || !pool->_tasks.empty();
			}
		);

		if (!getInstance()._workerNum && getInstance()._tasks.empty())
		{
			return;
		}
	}
}
