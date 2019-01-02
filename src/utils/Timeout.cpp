// Copyright © 2017-2019 Dmitriy Khaustov
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
// File created on: 2017.09.18

// Timeout.cpp


#include "Timeout.hpp"
#include "Daemon.hpp"

Timeout::Timeout(std::function<void()> handler, const char* label)
: realExpireTime(std::chrono::steady_clock::now())
, nextExpireTime(std::chrono::time_point<std::chrono::steady_clock>::max())
, _alarmHandler(std::move(handler))
, _label(label)
{
}

void Timeout::startOnce(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto now = std::chrono::steady_clock::now();

	if (realExpireTime > now)
	{
		return;
	}

	realExpireTime = now + duration;

	nextExpireTime = realExpireTime;

	if (!_timeoutWatcher)
	{
		_timeoutWatcher = std::make_shared<TimeoutWatcher>(ptr());
	}

	_timeoutWatcher->restart(nextExpireTime);
}

void Timeout::restart(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto now = std::chrono::steady_clock::now();
	realExpireTime = now + duration;

	auto prevExpireTime = nextExpireTime;
	nextExpireTime = std::min(realExpireTime, std::max(now, nextExpireTime));

	if (prevExpireTime <= nextExpireTime)
	{
		return;
	}

	if (!_timeoutWatcher)
	{
		_timeoutWatcher = std::make_shared<TimeoutWatcher>(ptr());
	}

	_timeoutWatcher->restart(nextExpireTime);
}

bool Timeout::operator()()
{
	if (realExpireTime > std::chrono::steady_clock::now() && !Daemon::shutingdown())
	{
		return false;
	}

	_mutex.unlock();
	_alarmHandler();
	return true;
}
