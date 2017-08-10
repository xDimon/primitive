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

#include <mutex>
#include <map>
#include <deque>
#include <condition_variable>
#include <queue>
#include "Task.hpp"
#include "Thread.hpp"

class ThreadPool final
{
public:
	ThreadPool(const ThreadPool&) = delete;
	void operator=(ThreadPool const&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

private:
	ThreadPool();
	virtual ~ThreadPool();

public:
	static ThreadPool& getInstance()
	{
		static ThreadPool instance;
		return instance;
	}

	static void setThreadNum(size_t num);

	static size_t genThreadId();

	static void enqueue(const std::shared_ptr<Task::Func>& function);
	static void enqueue(const std::shared_ptr<Task::Func>& function, Task::Duration delay);
	static void enqueue(const std::shared_ptr<Task::Func>& function, Task::Time time);
	static void enqueue(const std::shared_ptr<Task>& task);

	static void wait();

	static Thread* getCurrent();

	static void hold();
	static void unhold();

	static bool stops();

private:
	Log _log;

	std::mutex _counterMutex;
	size_t _workerCounter;
	size_t _workerNumber;

	size_t _hold;

	// need to keep track of threads so we can join them
	std::map<const std::thread::id, Thread*> _workers;

	// the task queue
	std::priority_queue<Task, std::deque<std::shared_ptr<Task>>> _tasks;

	// synchronization
	std::mutex _queueMutex;
	std::condition_variable _workersWakeupCondition;

	void createThread();
};
