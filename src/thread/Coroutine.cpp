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
// File created on: 2017.05.11

// Coroutine.cpp


#include <cstring>
#include "Coroutine.hpp"

#include "ThreadPool.hpp"

void Coro(std::function<void()> function)
{
	ucontext_t context;

	volatile bool done = false;

	// Инициализация контекста корутины. uc_link указывает на _parentContext, точку возврата при завершении корутины
	getcontext(&context);
//	context.uc_link = 0;
//	context.uc_stack.ss_flags = 0;
//	context.uc_stack.ss_sp = malloc(PTHREAD_STACK_MIN);
//	context.uc_stack.ss_size = PTHREAD_STACK_MIN;
//
//	// Заполнение _context, что позволяет swapcontext начать цикл.
//	// Преобразование в (void (*)(void)) необходимо для избежания  предупреждения компилятора и не влияет на поведение функции.
//	makecontext(&context, reinterpret_cast<void (*)(void)>(&_helper), 1, this);
//	log->debug("Make coroutine's helper context %p ", &_context);

	if (!done)
	{
		done = true;

		auto coro = new Coroutine(function, &context);
		coro->run();
		delete coro;
	}
}

Coroutine::Coroutine(std::function<void()> function, ucontext_t* parentContext)
: _function(std::move(function))
, _parentContext(parentContext)
{
	Log("Coroutine").debug("Coroutine created");
}

Coroutine::~Coroutine()
{
	Log("Coroutine").debug("Coroutine destroyed");
}

void Coroutine::_helper(Coroutine* coroutine)
{
	{
		Log log("CoroutineHelper");

		log.debug("Begin coroutine's helper");

//		ThreadPool::enqueue(
//			[coroutine]() {
//				coroutine->_mutex.lock();
//				coroutine->_mutex.unlock();
//
//				{
//					Log log("CoroutineTask");
//
//					log.debug("Begin coroutine's task");
//					coroutine->_function();
//					log.debug("End coroutine's task");
//
//					log.debug("Change context for continue prev thread %p", &coroutine->_parentContext);
//					std::this_thread::sleep_for(std::chrono::milliseconds(50));
//				}
//
//				if (setcontext(&coroutine->_parentContext))
//				{
//					throw std::runtime_error("Can't change context");
//				}
//			}
//		);
//
//		coroutine->_mutex.unlock();

		log.debug("End coroutine's helper");

//		log.debug("Change context to reenter thread %p", Thread::self()->reenterContext());
//		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	if (setcontext(coroutine->_parentContext))
	{
		throw std::runtime_error("Can't change context");
	}
};

void Coroutine::run()
{
	Log *log = new Log("CoroutineRun");

	log->debug("Begin coroutine's running");

	// Инициализация контекста корутины. uc_link указывает на _parentContext, точку возврата при завершении корутины
	getcontext(&_context);
	_context.uc_link = _parentContext;
	_context.uc_stack.ss_flags = 0;
	_context.uc_stack.ss_sp = _stack;
	_context.uc_stack.ss_size = PTHREAD_STACK_MIN;

	// Заполнение _context, что позволяет swapcontext начать цикл.
	// Преобразование в (void (*)(void)) необходимо для избежания  предупреждения компилятора и не влияет на поведение функции.
	makecontext(&_context, reinterpret_cast<void (*)(void)>(&_helper), 1, this);
	log->debug("Make coroutine's helper context %p ", &_context);

	_mutex.lock();

	log->debug("Swap context to coroutine helper");
//	int r41 = swapcontext(_parentContext, &_context);
	int r41 = setcontext(&_context);
	log->debug("swapcontext(%p->%p) return %d", &_parentContext, &_context, r41);

	log->debug("End coroutine's running");
}
