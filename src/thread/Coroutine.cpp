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


#include <ucontext.h>
#include <sys/mman.h>
#include "Coroutine.hpp"
#include "ThreadPool.hpp"

void Coroutine(std::function<void()> function)
{
	std::mutex orderMutex;
	ucontext_t mainContext;
	ucontext_t tmpContext;

	volatile bool first = true;

	// сохранить текущий контекст
	getcontext(&mainContext);

	// если это первый проход (т.е. не возврат)
	if (first)
	{
		first = false;

		{
			Log log("Coroutine");

			log.debug("Make coroutine");

			std::lock_guard<std::mutex> lockGuardCoro(orderMutex);

			// создать таск связанный с сохраненным контекстом и положить в очередь
			ThreadPool::enqueue(
				[&]()
				{
					{
						std::lock_guard<std::mutex> lockGuardTask(orderMutex);

						Log log("CoroutineTask");

						log.debug("Begin coroutine's task");
						function();
						log.debug("End coroutine's task");

						log.debug("Change context for continue prev thread %p", &mainContext);
//						std::this_thread::sleep_for(std::chrono::milliseconds(50));
					}

					// при завершении таска вернуться в связанный контекст, и удалить предыдущий
					if (swapcontext(&tmpContext, &mainContext))
					{
						throw std::runtime_error("Can't change context");
					}
				}
			);

			log.debug("Switch context (detach coroutine)");
		}

		// переключиться на контекст входа в поток
		Thread::self()->reenter();
	}
	else
	{
		Log log("Coroutine");

		log.debug("Switch context (reattach coroutine and continue)");

		munmap(tmpContext.uc_stack.ss_sp, tmpContext.uc_stack.ss_size);
	}

	return;
}
