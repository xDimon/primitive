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

// ThreadPool.hpp


#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <condition_variable>
#include <future>
#include <map>
#include "Thread.hpp"
#include "../log/Log.hpp"

class ThreadPool: public Log
{
private:
	ThreadPool();

	virtual ~ThreadPool();

	ThreadPool(ThreadPool const &) = delete;

	void operator=(ThreadPool const &) = delete;

public:
	static ThreadPool &getInstance()
	{
		static ThreadPool instance;
		return instance;
	}

	static void setThreadNum(size_t num);

	template<class F, class... Args>
	static auto enqueue(F &&f, Args &&... args)->std::future<typename std::result_of<F(Args...)>::type>;

	static void wait();

	static Thread *getCurrent();

private:
	size_t _workerNum;

	// need to keep track of threads so we can join them
	std::map<std::thread::id, Thread*> _workers;

	// the task queue
	std::queue<std::function<void()>> _tasks;

	// synchronization
	std::mutex _queue_mutex;
	std::condition_variable _condition;

	void createThread();
};

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args)->std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	std::future<return_type> res = task.get()->get_future();
	{
		std::unique_lock<std::mutex> lock(getInstance()._queue_mutex);

		// don't allow enqueueing after stopping the pool
		if (getInstance()._workerNum == 0)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		getInstance()._tasks.emplace(
			[task]() {
				task.get()->operator()();
			}
		);
	}

	getInstance()._condition.notify_all();
	return res;
}
