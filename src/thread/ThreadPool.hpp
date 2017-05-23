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
#include "Task.hpp"

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

	static size_t genThreadId();

	static void enqueue(Task&& task);
	static void enqueue(Task::Func function);
	static void enqueue(Task::Func function, Task::Duration delay);
	static void enqueue(Task::Func function, Task::Time time);

	static void wait();

	static Thread *getCurrent();

	static void hold();
	static void unhold();

private:
	std::mutex _counterMutex;
	size_t _workerCounter;
	size_t _workerNumber;

	size_t _hold;

	// need to keep track of threads so we can join them
	std::map<const std::thread::id, Thread*> _workers;

	// the task queue
	std::priority_queue<Task, std::deque<Task>> _tasks;

	// synchronization
	std::mutex _queueMutex;
	std::condition_variable _workersWakeupCondition;

	void createThread();
};
