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
// File created on: 2017.07.20

// TimeoutWatcher.cpp


#include "TimeoutWatcher.hpp"
#include "../thread/ThreadPool.hpp"
#include "TcpConnection.hpp"

TimeoutWatcher::TimeoutWatcher(const std::shared_ptr<Connection>& connection)
: _wp(connection)
, _refCounter(0)
{
};

void TimeoutWatcher::restart(std::chrono::steady_clock::time_point time)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(_wp.lock());
	if (!connection)
	{
		return;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(connection->mutex());
	++_refCounter;

	ThreadPool::enqueue(
		std::make_shared<std::function<void()>>(
			[p = ptr()](){
				(*p)();
			}
		),
		time
	);
}

void TimeoutWatcher::operator()()
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(_wp.lock());
	if (!connection)
	{
		Log("Timeout").info("Connection death");
		return;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(connection->mutex());
	_refCounter--;

	if (connection->expired())
	{
		Log("Timeout").info("Connection '%s' will be closed by timeout", connection->name().c_str());
		connection->close();
		return;
	}

	if (_refCounter == 0)
	{
		restart(connection->expireTime());
	}
}
