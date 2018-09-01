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

// Thread.cpp


#include "Thread.hpp"

#include "ThreadPool.hpp"
#include "../log/LoggerManager.hpp"
#include "RollbackStackAndRestoreContext.hpp"
#include "TaskManager.hpp"

#include <csignal>
#include <sys/mman.h>

thread_local Log Thread::_log("Thread");
thread_local std::function<void()> Thread::_function;

thread_local Thread* Thread::_self = nullptr;
thread_local ucontext_t* Thread::_currentContext = nullptr;
thread_local size_t Thread::_currentContextCount = 0;
thread_local ucontext_t* Thread::_obsoletedContext = nullptr;
thread_local ucontext_t* Thread::_replacedContext = nullptr;
thread_local std::queue<ucontext_t*> Thread::_replacedContexts;

thread_local ucontext_t* Thread::_contextPtrBuffer = nullptr;
thread_local ucontext_t* Thread::_currentTaskContextPtrBuffer = nullptr;

std::mutex Thread::_atCloseMutex;
std::deque<std::function<void ()>> Thread::_atCloseHandlers;

Thread::Thread(std::function<void()>& function)
: _thread((
	_mutex.lock(),
	[this, &function]
	{
		_id = ThreadPool::genThreadId();
		_function = std::move(function);
		_finished = false;
		Thread::run(this);
	}
))
{
	std::lock_guard<std::mutex> lockGuard(_mutex);
	_thread.detach();
	_log.debug("Thread 'Worker#%zu' created", _id);
}

Thread::~Thread()
{
	_log.debug("Thread 'Worker#%zu' destroyed", _id);
}

void Thread::run(Thread* thread)
{
	_self = thread;
	thread->_mutex.unlock();

	{
		char buff[32];
		snprintf(buff, sizeof(buff), "Worker#%zu", thread->id());

		LoggerManager::regThread(buff);

		// Блокируем реакцию на все сигналы
		sigset_t sig{};
		sigfillset(&sig);
		sigprocmask(SIG_BLOCK, &sig, nullptr);

//		thread->_log.info("Thread 'Worker#%zu' start", thread->_id);
	}

	do
	{
		//execute(thread);
		coroWrapper(thread, nullptr, nullptr);
	}
	while (thread->getCurrContextCount());

	{
		std::lock_guard<std::mutex> lockGuard(_atCloseMutex);
		for (const auto& handler : _atCloseHandlers)
		{
			handler();
		}
	}

//	thread->_log.info("Thread 'Worker#%zu' exit", thread->_id);

	LoggerManager::unregThread();

	thread->_finished = true;
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
	catch (...)
	{
		thread->_log.error("Uncatched exception on thread 'Worker#%zu'", thread->_id);
	}
}

void Thread::coroWrapper(Thread *thread, ucontext_t* context, std::mutex* mutex)
{
	if (mutex)
	{
		mutex->unlock();
	}

//	_self->_log.info("setCurentContext <= %p", context);

	auto prevContext = _currentContext;
	_currentContext = context;

	_currentContextCount++;
//	_self->_log.info("_currentContextCount++ => %zu", _currentContextCount);

	execute(thread);

	_currentContextCount--;
//	_self->_log.info("_currentContextCount-- => %zu", _currentContextCount);

	ThreadPool::getInstance()._contextsMutex.lock();
	auto contextForReplace = getContextForReplace();
	if (contextForReplace)
	{
		_obsoletedContext = context;

//		_self->_log.info("resetCurentContext =XX %p", _currentContext);
		_currentContext = prevContext;

		ThreadPool::getInstance()._contextsMutex.unlock();

		setcontext(contextForReplace);
	}
	ThreadPool::getInstance()._contextsMutex.unlock();
}

void Thread::yield(Task::Func&& func)
{
	volatile bool first = true;

	ucontext_t* context = nullptr;

	std::mutex orderMutex;
	ucontext_t retContext{};

	// Получить контекст
	getcontext(&retContext);

	if (first)
	{
		first = false;

		// Котнекствозврата в буфер потока, откуда его заберет конструктор задачи
		Thread::putContext(&retContext);

		//
		orderMutex.lock();

		// 4. Поместить задачу в очередь
		TaskManager::enqueue(
			[mutex = &orderMutex, &func]
			{
				// Синхронизируем смену контекста и выполнение задачи: задача только после смены контекста
				mutex->lock();
				mutex->unlock();

				func();
			},
			"yield"
		);

		context = new ucontext_t{};
		getcontext(context);
		context->uc_link = (ucontext_t*)0xDEAD;
		context->uc_stack.ss_flags = 0;
		context->uc_stack.ss_size = 1ull<<20; // PTHREAD_STACK_MIN;
		context->uc_stack.ss_sp = mmap(
			nullptr, context->uc_stack.ss_size,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANON, -1, 0
		);

		if (context->uc_stack.ss_sp == MAP_FAILED)
		{
			_log.warn("Can't to map memory for stack of context");

			delete context;

			throw std::runtime_error("Can't to map memory for stack of context");
		}

//		_log.info(
//			"Map memory for stack of context %p (%zu on %p) => %lld",
//			context,
//			context->uc_stack.ss_size,
//			context->uc_stack.ss_sp
//		);

		makecontext(context, reinterpret_cast<void (*)()>(coroWrapper), sizeof(void*) * 3 / sizeof(int), this, context, &orderMutex);

		if (setcontext(context))
		{
			throw std::runtime_error("Can't change context");
		}
	}

	if (_obsoletedContext)
	{
//		_log.info(
//			"UnMap memory for stack of context %p (%zu on %p)",
//			_obsoletedContext,
//			_obsoletedContext->uc_stack.ss_size,
//			_obsoletedContext->uc_stack.ss_sp
//		);

		munmap(_obsoletedContext->uc_stack.ss_sp, _obsoletedContext->uc_stack.ss_size);
		delete _obsoletedContext;
		_obsoletedContext = nullptr;
	}
}

void Thread::setCurrTaskContext(ucontext_t* context)
{
	if (_currentTaskContextPtrBuffer && context)
	{
		throw;
	}
	_currentTaskContextPtrBuffer = context;
//	_self->_log.info("setCurrTaskContext <= %p", context);
}

ucontext_t* Thread::getCurrTaskContext()
{
	ucontext_t* context = _currentTaskContextPtrBuffer;
//	_self->_log.info("getCurrTaskContext => %p", context);
	return context;
}

void Thread::putContext(ucontext_t* context)
{
	_contextPtrBuffer = context;
//	_self->_log.info("putContext <= %p", context);
}

// Извлекаем указатель на контекст из буффера потока
ucontext_t* Thread::getContext()
{
	ucontext_t* context = _contextPtrBuffer;
	_contextPtrBuffer = nullptr;
//	if (_self)
//	{
//		_self->_log.info("getContext => %p", context);
//	}
	return context;
}

void Thread::putContextForReplace(ucontext_t* context)
{
	_replacedContexts.emplace(context);
//	_self->_log.info("setContextForReplace <= %p (%zu)", context, _replacedContexts.size());
}

ucontext_t* Thread::getContextForReplace()
{
	ucontext_t* context = _replacedContexts.front();
	_replacedContexts.pop();

//	_self->_log.info("getContextForReplace => %p (%zu)", context, _replacedContexts.size());
	return context;
}

size_t Thread::sizeContextForReplace()
{
	return _replacedContexts.size();
}

size_t Thread::getCurrContextCount()
{
//	_self->_log.info("Thread 'Worker#%zu' CurrContextCount=%zu", _self->_id, _currentContextCount);

	return _currentContextCount;
}

void Thread::postpone(Task::Duration duration)
{
	auto ctx = Thread::getCurrTaskContext();
	Thread::setCurrTaskContext(nullptr);

	TaskManager::enqueue(
		[ctx]
		{
			throw RollbackStackAndRestoreContext(ctx);
		},
		duration,
		"Postpone some thing"
	);
}

void Thread::doAtShutdown(std::function<void()>&& func)
{
	std::lock_guard<std::mutex> lockGuard(_atCloseMutex);

	_atCloseHandlers.emplace_back(std::forward<std::function<void()>>(func));
}
