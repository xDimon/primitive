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
#include "RollbackStackAndRestoreContext.hpp"

#include <sys/mman.h>
#include <ucontext.h>
#include <climits>

thread_local Thread::Id Thread::_tid;

Thread::Thread(std::function<void()>& function)
: _id(ThreadPool::genThreadId())
, _log("Thread")
, _function(std::move(function))
, _finished((_mutex.lock(), false))
, _thread([this]() { Thread::run(this); })
{
	_thread.detach();
	_log.debug("Thread 'Worker#%zu' created", _id);
}

Thread::~Thread()
{
	_log.debug("Thread 'Worker#%zu' destroyed", _id);
}

void Thread::coroutine(Thread* thread)
{
	try
	{
		thread->_function();
	}
	catch (const RollbackStackAndRestoreContext& exception)
	{}
	catch (const std::exception& exception)
	{
		thread->_log.error("Uncatched exception on thread 'Worker#%zu': %s", thread->_id, exception.what());
	}
}

void Thread::run(Thread* thread)
{
	_tid = thread->_id;
	thread->_mutex.unlock();

	{
		char buff[32];
		snprintf(buff, sizeof(buff), "Worker#%zu", thread->_id);

		LoggerManager::regThread(buff);

		// Блокируем реакцию на все сигналы
		sigset_t sig{};
		sigfillset(&sig);
		sigprocmask(SIG_BLOCK, &sig, nullptr);

		thread->_log.debug("Thread 'Worker#%zu' start", thread->_id);
	}

	thread->reenter();

	thread->_finished = true;

	LoggerManager::unregThread();
}

Thread* Thread::self()
{
	auto tid = Thread::_tid;
	return ThreadPool::getThread(tid);
}

void Thread::reenter()
{
	ucontext_t context{};
	ucontext_t retContext{};

	// Инициализация контекста. uc_link указывает на точку возврата при выходе из стека
	getcontext(&context);
	context.uc_link = &retContext;
	context.uc_stack.ss_flags = 0;
	context.uc_stack.ss_size = 1<<20;// PTHREAD_STACK_MIN;
	context.uc_stack.ss_sp = mmap(
		nullptr, context.uc_stack.ss_size,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANON, -1, 0
	);


	if (context.uc_stack.ss_sp == MAP_FAILED)
	{
		_log.warn("Can't to map memory for stack of context");
		return;
	}

	makecontext(&context, reinterpret_cast<void (*)()>(coroutine), 1, this);

	volatile bool first = true;

	getcontext(&retContext);

	if (first)
	{
		first = false;

		if (setcontext(&context))
		{
			throw std::runtime_error("Can't change context");
		}
	}

	munmap(context.uc_stack.ss_sp, context.uc_stack.ss_size);
	context.uc_stack.ss_sp = nullptr;
	context.uc_stack.ss_size = 0;
	context.uc_link = nullptr;
}
