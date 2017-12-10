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
// File created on: 2017.02.26

// ThreadPool.cpp


#include <ucontext.h>
#include <sys/mman.h>
#include "ThreadPool.hpp"
#include "../utils/Time.hpp"
#include "RollbackStackAndRestoreContext.hpp"
#include "../utils/Daemon.hpp"

// the constructor just launches some amount of _workers
ThreadPool::ThreadPool()
: _log("ThreadPool")
, _workerCounter(0)
{
}

void ThreadPool::hold()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	++pool._hold;
	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::unhold()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	--pool._hold;
}

void ThreadPool::setThreadNum(size_t num)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	size_t remain = (num < pool._workers.size()) ? 0 : (num - pool._workers.size());
	while (remain-- > 0)
	{
		pool.createThread();
	}

	pool._workersWakeupCondition.notify_one();
}

size_t ThreadPool::genThreadId()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._counterMutex);
	return ++pool._workerCounter;
}

void ThreadPool::createThread()
{
	std::function<void()> threadLoop = [this]()
	{
		Task::Time waitUntil = std::chrono::steady_clock::now() + std::chrono::hours(24);

		auto continueCondition = [this,&waitUntil](){
			std::lock_guard<std::mutex> lockGuard(_counterMutex);
			if (Daemon::shutingdown())
			{
				return true;
			}
			if (_tasks.empty())
			{
				return _hold == 0;
			}
			waitUntil = _tasks.top()->until();
			return waitUntil <= std::chrono::steady_clock::now();
		};

		_log.debug("Begin thread's loop");

		std::shared_ptr<Task> task;

		for (;;)
		{
			task.reset();

			{
				std::unique_lock<std::mutex> lock(_queueMutex);

				// Condition for run thread
				if (!_workersWakeupCondition.wait_until(lock, waitUntil, continueCondition))
				{
					waitUntil = std::chrono::steady_clock::now() + std::chrono::hours(24);
					continue;
				}

				// Condition for end thread
				if (_tasks.empty())
				{
					_log.debug("End thread's loop");
					break;
				}

				_log.trace("Get task from queue");

				// Get task from queue
				task = _tasks.top();
				_tasks.pop();

				// Condition for end thread
				if (!_tasks.empty())
				{
					_workersWakeupCondition.notify_one();

					static time_t pt = 0;
					time_t t = time(nullptr);
					if (pt != t)
					{
						pt = t;
						_log.info("Task queue length:           %llu", _tasks.size());
					}
				}
			}

			{
				static time_t pt = 0;
				time_t t = time(nullptr);
				if (pt != t)
				{
					pt = t;

					std::lock_guard<std::mutex> lockGuard(_contextsMutex);

					_log.info("Waiting contexts:            %llu", _waitingContexts.size());
					_log.info("Ready for continue contexts: %llu", _readyForContinueContexts.size());
				}
			}

			// Execute task
			_log.trace("Begin execution task on thread");

			bool done = true;
			try
			{
				done = (*task)();
			}
			catch (const RollbackStackAndRestoreContext& exception)
			{
			}
			catch (const std::exception& exception)
			{
				_log.warn("Uncatched exception at execute task of pool: %s", exception.what());
			}
			if (!done)
			{
				_log.trace("Reenqueue task");

				std::lock_guard<std::mutex> lockGuard(_queueMutex);
				waitUntil = task->until();
				_tasks.emplace(std::move(task));
			}
			else
			{
				task->restoreCtx();

				_log.trace("End execution task on thread");

				std::lock_guard<std::mutex> lockGuard(_contextsMutex);
				if (Thread::onSubContext())
				{
					_log.info("Has subContext");
					if (!_readyForContinueContexts.empty())
					{
						auto ctx = _readyForContinueContexts.front();
						_readyForContinueContexts.pop();

						Thread::self()->replacedContext() = ctx;

						_log.info("Context %p: Set for replace (%llu on %p)", ctx, ctx->uc_stack.ss_size, ctx->uc_stack.ss_sp);

//						_log.debug("End of secondary task");

						_workersWakeupCondition.notify_one();

						break;
					}
				}
				else
				{
					_log.info("Hasn't subContext");
				}
			}

			_log.trace("Waiting for task on thread");
		}

		_workersWakeupCondition.notify_one();
	};

	_log.debug("Thread create");

	auto thread = new Thread(threadLoop);

	thread->waitStart();

	_workers.emplace(thread->id(), thread);

	_workersWakeupCondition.notify_one();
}

void ThreadPool::wait()
{
	auto& pool = getInstance();

	pool._log.debug("Waiting for threadpool close");

	// Condition for close pool
	for(;;)
	{
		// Wait end all threads
		{
			std::lock_guard<std::mutex> lockGuard(pool._queueMutex);
			for (auto i = pool._workers.begin(); i != pool._workers.end(); )
			{
				auto ci = i++;
				Thread *thread = ci->second;
				if (thread->finished())
				{
					pool._workers.erase(ci);
					delete thread;
				}
			}
			if (pool._workers.empty())
			{
				break;
			}
			if (Daemon::shutingdown())
			{
				pool._workersWakeupCondition.notify_all();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

Thread *ThreadPool::getThread(Thread::Id tid)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	auto i = pool._workers.find(tid);
	if (i == pool._workers.end())
	{
		throw std::runtime_error("Unrecognized thread");
	}
	return i->second;
}

void ThreadPool::enqueue(const std::shared_ptr<Task>& task)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	pool._tasks.emplace(task);

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function));

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function, Task::Duration delay)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function, delay));

	pool._workersWakeupCondition.notify_one();
}

void ThreadPool::enqueue(const std::shared_ptr<Task::Func>& function, Task::Time time)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	pool._tasks.emplace(std::make_shared<Task>(function, time));

	pool._workersWakeupCondition.notify_one();
}

bool ThreadPool::empty()
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._queueMutex);

	return pool._workers.empty();
}

uint64_t ThreadPool::postponeContext(ucontext_t* context)
{
	static uint64_t ctxId = 0;

	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._contextsMutex);

	pool._waitingContexts.emplace(++ctxId, context);
//	pool._log.info("postponeContext %llu", ctxId);

	return ctxId;
}

void ThreadPool::continueContext(uint64_t ctxId)
{
	auto& pool = getInstance();

	std::lock_guard<std::mutex> lockGuard(pool._contextsMutex);

	auto i = pool._waitingContexts.find(ctxId);
	if (i == pool._waitingContexts.end())
	{
		throw std::runtime_error("Context Id not found");
	}

//	pool._log.info("continueContext %llu", ctxId);
	pool._readyForContinueContexts.emplace(i->second);

	pool._waitingContexts.erase(i);
}
