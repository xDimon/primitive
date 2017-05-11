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


#include "Coroutine.hpp"

#include "ThreadPool.hpp"

Coroutine::Coroutine(std::function<void()> function)
: Log("Coroutine")
, _function(function)
{
	log().debug("Coroutine created");
}

Coroutine::~Coroutine()
{
	log().debug("Coroutine destroyed");
}

void Coroutine::_helper(Coroutine* coroutine)
{
	Log log("_helper");

	log.debug("Coroutine #%u", __LINE__);
	ThreadPool::enqueue(
		[coroutine,&log]() {
			log.debug("Coroutine #%u", __LINE__);
			coroutine->_function();
			log.debug("Coroutine #%u", __LINE__);
			swapcontext(&coroutine->_context, &coroutine->_parentContext);
//			setcontext(&coroutine->_parentContext);
			log.debug("Coroutine #%u", __LINE__);
		}
	);

	log.debug("Coroutine #%u", __LINE__);
	setcontext(Thread::self()->reenterContext());
};

void Coroutine::run()
{
	log().debug("Coroutine enter");

	log().debug("Coroutine #%u", __LINE__);

	// Инициализация контекста корутины. uc_link указывает на _parentContext, точку возврата при завершении корутины
	getcontext(&_context);
	_context.uc_link = &_parentContext;
	_context.uc_stack.ss_sp = _stack;
	_context.uc_stack.ss_size = sizeof(_stack);

	// Заполнение _context, что позволяет swapcontext начать цикл.
	// Преобразование в (void (*)(void)) необходимо для избежания  предупреждения компилятора и не влияет на поведение функции.
	makecontext(&_context, reinterpret_cast<void (*)(void)>(&_helper), 1, this);

	_done = false;

	log().debug("Coroutine #%u", __LINE__);

	// Сохранение текущего контекста в _parentContext. При завершении корутины управление будет возвращено в эту точку
	getcontext(&_parentContext);

	log().debug("Coroutine #%u", __LINE__);

	if (!_done)
	{
		_done = true;

		log().debug("Coroutine #%u", __LINE__);

//		swapcontext(&_context, &_parentContext);
//		swapcontext(&_parentContext, &_context);
		setcontext(&_context);

		log().debug("Coroutine #%u", __LINE__);
	}
	else
	{
		log().debug("Coroutine #%u", __LINE__);
	}

	log().debug("Coroutine leave");
}
