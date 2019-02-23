// Copyright © 2018-2019 Dmitriy Khaustov
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
// File created on: 2018.10.29

// Timer.cpp


#include "Timer.hpp"
#include "../thread/TaskManager.hpp"
#include "Daemon.hpp"

Timer::Helper::Helper(const std::shared_ptr<Timer>& timer)
: _wp(timer)
, _refCounter(0)
{
}

void Timer::Helper::onTime()
{
	auto timeout = _wp.lock();
	if (!timeout)
	{
		return;
	}

	timeout->_mutex.lock();

	_refCounter--;

	if (timeout->alarm())
	{
		return;
	}

	if (_refCounter == 0)
	{
		start(timeout->_actualAlarmTime);
	}

	timeout->_mutex.unlock();
}

void Timer::Helper::start(std::chrono::steady_clock::time_point time)
{
	auto timeout = _wp.lock();
	if (!timeout)
	{
		return;
	}

	++_refCounter;

	TaskManager::enqueue(
		[p = ptr()]
		{
			p->onTime();
		},
		time,
		timeout->label()
	);
}

void Timer::Helper::cancel()
{
	auto timer = _wp.lock();
	if (!timer)
	{
		return;
	}

	timer->_helper.reset();
	_wp.reset();
}


Timer::Timer(std::function<void()> handler, const char* label)
: _label(label)
, _handler(std::move(handler))
, _actualAlarmTime(std::chrono::steady_clock::now())
, _nextAlarmTime(std::chrono::time_point<std::chrono::steady_clock>::max())
{
}

bool Timer::alarm()
{
	if (_actualAlarmTime > std::chrono::steady_clock::now() && !Daemon::shutingdown())
	{
		return false;
	}

	_mutex.unlock();
	_handler();
	return true;
}

Timer::AlarmTime Timer::appoint(AlarmTime currentTime, AlarmTime alarmTime, bool once)
{
	if (once)
	{
		if (_actualAlarmTime > currentTime)
		{
			return _actualAlarmTime;
		}

		_actualAlarmTime = alarmTime;

		_nextAlarmTime = _actualAlarmTime;
	}
	else
	{
		_actualAlarmTime = alarmTime;

		auto prevExpireTime = _nextAlarmTime;

		if (_actualAlarmTime < std::max(currentTime, _nextAlarmTime))
		{
			_nextAlarmTime = _actualAlarmTime;
		}
		else
		{
			_nextAlarmTime = std::max(currentTime, _nextAlarmTime);
		}

		if (prevExpireTime <= _nextAlarmTime)
		{
			return _actualAlarmTime;
		}
	}

	if (!_helper)
	{
		_helper = std::make_shared<Helper>(ptr());
	}

	_helper->start(_nextAlarmTime);

	return _actualAlarmTime;
}

Timer::AlarmTime Timer::start(std::chrono::microseconds duration, bool once)
{
	std::lock_guard<mutex_t> lockGuard(_mutex);
	auto now = std::chrono::steady_clock::now();
	return appoint(now, now + duration, once);
}

Timer::AlarmTime Timer::startOnce(std::chrono::microseconds duration)
{
	std::lock_guard<mutex_t> lockGuard(_mutex);
	auto now = std::chrono::steady_clock::now();
	return appoint(now, now + duration, true);
}

Timer::AlarmTime Timer::restart(std::chrono::microseconds duration)
{
	std::lock_guard<mutex_t> lockGuard(_mutex);
	auto now = std::chrono::steady_clock::now();
	return appoint(now, now + duration, false);
}

Timer::AlarmTime Timer::prolong(std::chrono::microseconds duration)
{
	std::lock_guard<mutex_t> lockGuard(_mutex);

	auto now = std::chrono::steady_clock::now();
	auto alarmTime = now + duration;

	if (_actualAlarmTime >= alarmTime)
	{
		return _actualAlarmTime;
	}

	return appoint(now, alarmTime, false);
}

Timer::AlarmTime Timer::shorten(std::chrono::microseconds duration)
{
	std::lock_guard<mutex_t> lockGuard(_mutex);

	auto now = std::chrono::steady_clock::now();
	auto alarmTime = std::chrono::steady_clock::now() + duration;

	if (_actualAlarmTime <= alarmTime)
	{
		return _actualAlarmTime;
	}

	return appoint(now, alarmTime, false);
}

void Timer::stop()
{
	std::lock_guard<mutex_t> lockGuard(_mutex);

	_helper->cancel();
}
