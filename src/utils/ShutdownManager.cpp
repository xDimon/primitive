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
// File created on: 2017.05.21

// ShutdownManager.cpp


#include "ShutdownManager.hpp"
#include "../thread/ThreadPool.hpp"

void ShutdownManager::doAtShutdown(std::function<void()> handler)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	getInstance()._handlers.emplace_front(std::move(handler));
}

void ShutdownManager::shutdown()
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

//	ThreadPool::hold();

	getInstance()._shutingdown = true;

	for (auto handler : getInstance()._handlers)
	{
		handler();
	}

//	ThreadPool::unhold();

}
