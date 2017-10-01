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
#include "Coroutine.hpp"
#include "ThreadPool.hpp"
#include "../utils/Daemon.hpp"

void Coroutine(const std::shared_ptr<Task>& task)
{
	if (Daemon::shutingdown())
	{
		return;
	}

	std::mutex orderMutex;
	ucontext_t mainContext{};
	ucontext_t tmpContext{};

	volatile bool first = true;

	// сохранить указатели на контексты
	task->saveContext(&tmpContext, &mainContext);

	// сохранить текущий контекст
	getcontext(&mainContext);

	// если это первый проход (т.е. не возврат)
	if (first)
	{
		first = false;
		{
			Log("Coroutine").debug("Make coroutine");

			std::lock_guard<std::mutex> lockGuardCoro(orderMutex);

			// создать таск связанный с сохраненным контекстом и положить в очередь

			auto taskWrapper = std::make_shared<Task::Func>(
				[&orderMutex, wp = std::weak_ptr<Task>(task)]()
				{
					std::lock_guard<std::mutex> lockGuardTask(orderMutex);

					auto task = wp.lock();
					if (!task)
					{
						return;
					}

					try
					{
						(*task)();
					}
					catch (const std::exception& exception)
					{
						Log("Coroutine").warn("Exception at execute task of coroutine: %s", exception.what());
					}
				}
			);

			ThreadPool::enqueue(taskWrapper);
		}

		// переключиться на контекст входа в поток
		Thread::self()->reenter();
	}
}
