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
// File created on: 2017.02.28

// Thread.hpp


#pragma once

#include "../log/Log.hpp"
#include <mutex>

class Task;

class Thread
{
public:
	typedef size_t Id;

private:
	std::mutex _mutex;
	Id _id;
	Log _log;
	std::function<void()> _function;
	bool _finished;
	std::thread _thread;

	static thread_local Id _tid;

	static void run(Thread*);

public:
	static void coroutine(Thread*);

public:
	Thread() = delete;
	Thread(const Thread&) = delete; // Copy-constructor
	void operator=(Thread const&) = delete; // Copy-assignment
	Thread(Thread&&) = delete; // Move-constructor
	Thread& operator=(Thread&&) = delete; // Move-assignment

	explicit Thread(std::function<void()>& threadLoop);
	virtual ~Thread();

	static Thread* self();

	inline void waitStart()
	{
		std::lock_guard<std::mutex> lockGuard(_mutex);
	}

	inline bool finished()
	{
		return _finished;
	}

	inline auto id() const
	{
		return _id;
	}

	void reenter();

	void yield(const std::shared_ptr<Task>& task);
};
