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

// Thread.cpp


#include "Thread.hpp"

#include "ThreadPool.hpp"
#include "../log/LoggerManager.hpp"

#include <cstring>
#include <sys/mman.h>
#include <signal.h>
#include <ucontext.h>
#include <climits>

Thread::Thread(std::function<void()> function)
: _id(ThreadPool::genThreadId())
, _log("Thread")
, _function(std::move(function))
, _thread([this](){ Thread::run(this); })
, _finished(false)
{
	_log.debug("Thread 'Worker_%zu' created", _id);
}

Thread::~Thread()
{
	_log.debug("Thread 'Worker_%zu' destroyed", _id);
}

void fake(Thread *thread)
{
	thread->_function();
}

void Thread::run(Thread *thread)
{
	{
		char buff[32];
		snprintf(buff, sizeof(buff), "Worker_%zu", thread->_id);

		LoggerManager::regThread(buff);

		// Блокируем реакцию на все сигналы
		sigset_t sig;
		sigfillset(&sig);
		sigprocmask(SIG_BLOCK, &sig, nullptr);

		thread->_log.debug("Thread 'Worker_%zu' start", thread->_id);
	}

#if WITH_COROUTINE
	thread->reenter();
#else
	fake(thread);
#endif

	LoggerManager::unregThread();

	thread->_finished = true;
}

Thread* Thread::self()
{
	return ThreadPool::getCurrent();
}

void Thread::reenter()
{
	ucontext_t context;
	ucontext_t retContext;

//	_log.debug("make context");

	// Инициализация контекста корутины. uc_link указывает на _parentContext, точку возврата при завершении корутины
	getcontext(&context);
	context.uc_link = &retContext;
	context.uc_stack.ss_flags = 0;
	context.uc_stack.ss_size = PTHREAD_STACK_MIN;
	context.uc_stack.ss_sp = mmap(0, context.uc_stack.ss_size,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANON, -1, 0);

	makecontext(&context, reinterpret_cast<void (*)(void)>(fake), 1, this);
//	makecontext(&context, reinterpret_cast<void (*)(void)>(fake), 0);

//	_log.debug("get ret context %p", &retContext);

	volatile bool first = true;

	getcontext(&retContext);

	if (first)
	{
		first = false;
//		_log.debug("first set context %p", &context);

		if (setcontext(&context))
		{
			throw std::runtime_error("Can't change context");
		}

//		_log.debug("return from first context %p", &context);
	}

//	_log.debug("free stack of first context %p", &context);

	munmap(context.uc_stack.ss_sp, context.uc_stack.ss_size);
	context.uc_stack.ss_sp = nullptr;
	context.uc_stack.ss_size = 0;
}
