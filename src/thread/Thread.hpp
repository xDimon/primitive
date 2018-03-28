// Copyright © 2017-2018 Dmitriy Khaustov
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

#include <functional>
#include <mutex>
#include <ucontext.h>
#include <stack>
#include <queue>
#include "../log/Log.hpp"
#include "Task.hpp"

class Thread
{
public:
	typedef size_t Id;

private:
	std::mutex _mutex;
	Id _id;

	static thread_local Log _log;
	static thread_local std::function<void()> _function;
	bool _finished;
	std::thread _thread;

public:// TODO временно
	static thread_local Thread* _self;
	static thread_local ucontext_t* _currentContext;
	static thread_local size_t _currentContextCount;
	static thread_local ucontext_t* _obsoletedContext;
	static thread_local ucontext_t* _replacedContext;
	static thread_local std::queue<ucontext_t*> _replacedContexts;

	static thread_local ucontext_t* _contextPtrBuffer;
	static thread_local ucontext_t* _currentTaskContextPtrBuffer;

	static void run(Thread* thread);

public:
	static void execute(Thread* thread);

public:
	Thread() = delete;
	Thread(const Thread&) = delete; // Copy-constructor
	Thread& operator=(Thread const&) = delete; // Copy-assignment
	Thread(Thread&&) noexcept = delete; // Move-constructor
	Thread& operator=(Thread&&) noexcept = delete; // Move-assignment

	explicit Thread(std::function<void()>& threadLoop);
	virtual ~Thread();

	static Thread* self()
	{
		return _self;
	}

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

	// Кладем указатель на контекст в буффер потока
	static void putContext(ucontext_t* context);
	// Извлекаем указатель на контекст из буффера потока
	static ucontext_t* getContext();

	static void setCurrTaskContext(ucontext_t* context);
	static ucontext_t* getCurrTaskContext();

	static void putContextForReplace(ucontext_t* context);
	static ucontext_t* getContextForReplace();
	static size_t sizeContextForReplace();

	static size_t getCurrContextCount();

	void yield(Task::Func&& func);

	template<class F>
	void yield(F& func)
	{
		yield([&func]{ func(); });
	}

	void postpone(Task::Duration duration);

	static void coroWrapper(Thread* thread, ucontext_t* context, std::mutex* mutex);

private:
	static std::mutex _atCloseMutex;
	static std::deque<std::function<void ()>> _atCloseHandlers;
public:
	static void doAtShutdown(std::function<void ()>&& handler);
};
