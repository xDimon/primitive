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

// TaskManager.hpp


#pragma once


#include <memory>
#include <deque>
#include <queue>
#include <mutex>
#include <set>
#include "Task.hpp"
#include "../log/Log.hpp"

class TaskManager final
{
public:
	TaskManager(const TaskManager&) = delete; // Copy-constructor
	void operator=(TaskManager const&) = delete; // Copy-assignment
	TaskManager(TaskManager&&) = delete; // Move-constructor
	TaskManager& operator=(TaskManager&&) = delete; // Move-assignment

private:
	TaskManager();
	~TaskManager() = default;

public:
	static TaskManager& getInstance()
	{
		static TaskManager instance;
		return instance;
	}

private:
	Log _log;
	std::mutex _mutex;
	std::priority_queue<Task, std::deque<Task>> _queue;

public:
	static void enqueue(Task::Func&& func, Task::Time time);

	static void enqueue(Task::Func&& func, Task::Duration delay)
	{
		enqueue(std::forward<Task::Func>(func), Task::Clock::now() + delay);
	}

	static void enqueue(Task::Func&& func)
	{
		enqueue(std::forward<Task::Func>(func), Task::Clock::now());
	}

//	template<class F, class... S>
//	static typename std::enable_if<std::is_convertible<F, Task::Func>::value, void>::type
//	enqueue(F&& func, const S&... spec)
//	{
//		enqueue(std::forward<Task::Func>(func), spec...);
//	}

	static size_t queueSize();

	static Task::Time waitUntil();

	static void executeOne();

	static bool empty();
};
