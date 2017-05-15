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
#include "Coroutine.hpp"

#include <cstring>

Thread::Thread(std::function<void()> function)
: Log("Thread")
, _function(std::move(function))
, _thread([this](){ Thread::run(this); })
{
	log().debug("Thread created");
}

Thread::~Thread()
{
	log().debug("Thread destroyed");
}

void Thread::run(Thread *thread)
{
	// Блокируем реакцию на все сигналы
	sigset_t sig;
	sigfillset(&sig);
	sigprocmask(SIG_BLOCK, &sig, nullptr);

	Coro(thread->_function);
}

Thread* Thread::self()
{
	return ThreadPool::getCurrent();
}
