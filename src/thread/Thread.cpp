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

#include <csignal>
#include <sys/mman.h>

thread_local Thread::Id Thread::_tid;
thread_local ucontext_t* Thread::_currentContext = nullptr;
thread_local ucontext_t* Thread::_obsoletedContext = nullptr;
thread_local ucontext_t* Thread::_replacedContext = nullptr;

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

	execute(thread);

	thread->_finished = true;

	thread->_log.debug("Thread 'Worker#%zu' exit", thread->_id);

	LoggerManager::unregThread();
}

Thread* Thread::self()
{
	auto tid = Thread::_tid;
	return ThreadPool::getThread(tid);
}

void Thread::execute(Thread* thread)
{
	try
	{
		thread->_function();
	}
	catch (const RollbackStackAndRestoreContext& exception)
	{
		thread->_log.error("RollbackStackAndRestoreContext on thread 'Worker#%zu': %s", thread->_id, exception.what());
	}
	catch (const std::exception& exception)
	{
		thread->_log.error("Uncatched exception on thread 'Worker#%zu': %s", thread->_id, exception.what());
	}
}

void Thread::coroWrapper(Thread *thread, ucontext_t* context, std::mutex* mutex)
{
	if (mutex)
	{
		mutex->unlock();
	}

	_currentContext = context;

	execute(thread);

	if (_replacedContext)
	{
		auto ctx = _replacedContext;
		_replacedContext = nullptr;

		_obsoletedContext = context;

		_currentContext = nullptr;

		setcontext(ctx);
	}
}

ucontext_t*& Thread::replacedContext()
{
	return _replacedContext;
}

void Thread::yield(const std::shared_ptr<Task>& task)
{
	if (!task)
	{
		return;
	}

	volatile bool first = true;

	auto orderMutex = new std::mutex{};
	auto context = new ucontext_t{};
	auto retContext = new ucontext_t{};

	// Получить контекст
	getcontext(retContext);

	if (first)
	{
		first = false;

		// 1. Сохранить контекст и Получить его номер, чтоб мочь продолжить его
		uint64_t ctxId = ThreadPool::postponeContext(retContext);

		// 3. Создать задачу, так, чтоб она знала контекст для возврата
		task->saveCtx(ctxId);

		auto taskWrapper = std::make_shared<Task::Func>(
			[&orderMutex, wp = std::weak_ptr<Task>(task)]
			{
				{ std::lock_guard<std::mutex> lockGuardInnerCoro(*orderMutex); }

				auto originalTask = wp.lock();
				if (!originalTask)
				{
					return;
				}

				(*originalTask)();

				originalTask->restoreCtx();
			}
		);

		orderMutex->lock();

		// 4. Поместить задачу в очередь
		ThreadPool::enqueue(taskWrapper);

		getcontext(context);
		context->uc_link = (ucontext_t*)0xDEAD;
		context->uc_stack.ss_flags = 0;
		context->uc_stack.ss_size = 1<<20; // PTHREAD_STACK_MIN;
		context->uc_stack.ss_sp = mmap(
			nullptr, context->uc_stack.ss_size,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANON, -1, 0
		);

		if (context->uc_stack.ss_sp == MAP_FAILED)
		{
			_log.warn("Can't to map memory for stack of context");

			delete orderMutex;
			delete context;
			delete retContext;

			return;
		}

		_log.info("Map memory for stack of context (%lld on %p)", context->uc_stack.ss_size, context->uc_stack.ss_sp);

		makecontext(context, reinterpret_cast<void (*)()>(coroWrapper), sizeof(void*) * 3 / sizeof(int), this, context, orderMutex);

		if (setcontext(context))
		{
			throw std::runtime_error("Can't change context");
		}
	}
	else
	{
		if (_obsoletedContext)
		{
			_log.info("UnMap memory for stack of context (%lld on %p)", _obsoletedContext->uc_stack.ss_size, _obsoletedContext->uc_stack.ss_sp);

			munmap(_obsoletedContext->uc_stack.ss_sp, _obsoletedContext->uc_stack.ss_size);
			delete _obsoletedContext;
			_obsoletedContext = nullptr;
		}
		delete orderMutex;
		delete retContext;
	}
}
