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

Timer::Helper::Helper(const std::shared_ptr<Timer>& timer)
: _wp(timer)
, _time(std::chrono::steady_clock::duration::zero())
{
}

void Timer::Helper::onTime()
{
	auto timer = _wp.lock();
	if (!timer)
	{
		return;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(timer->_mutex);

	timer->alarm();
}

void Timer::Helper::start(std::chrono::steady_clock::time_point time)
{
	auto timer = _wp.lock();
	if (!timer)
	{
		return;
	}

	_time = time;

	TaskManager::enqueue(
		[wp = std::weak_ptr<Timer::Helper>(ptr())]
		{
			if (auto iam = wp.lock())
			{
				iam->onTime();
			}
		},
		time,
		timer->label()
	);
}

void Timer::Helper::cancel()
{
	auto timer = _wp.lock();
	if (!timer)
	{
		return;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(timer->_mutex);

	_wp.reset();
}

Timer::Timer(std::function<void()> handler, const char* label)
: _alarmHandler(std::move(handler))
, _label(label)
{
}

void Timer::alarm()
{
	_helper.reset();
	_alarmHandler();
}

Timer::AlarmTime Timer::startOnce(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (!_helper)
	{
		Timer::AlarmTime alarmTime = std::chrono::steady_clock::now() + duration;

		_helper = std::make_shared<Helper>(ptr());

		_helper->start(alarmTime);
	}

	return _helper->time();
}

Timer::AlarmTime Timer::restart(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	Timer::AlarmTime alarmTime = std::chrono::steady_clock::now() + duration;

	if (_helper)
	{
		_helper->cancel();
	}

	_helper = std::make_shared<Helper>(ptr());

	_helper->start(alarmTime);

	return alarmTime;
}

Timer::AlarmTime Timer::prolong(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	Timer::AlarmTime alarmTime = std::chrono::steady_clock::now() + duration;

	if (_helper)
	{
		if (_helper->time() > alarmTime)
		{
			return _helper->time();
		}

		_helper->cancel();
	}

	_helper = std::make_shared<Helper>(ptr());

	_helper->start(alarmTime);

	return alarmTime;
}

Timer::AlarmTime Timer::shorten(std::chrono::milliseconds duration)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	Timer::AlarmTime alarmTime = std::chrono::steady_clock::now() + duration;

	if (_helper)
	{
		if (_helper->time() <= alarmTime)
		{
			return _helper->time();
		}

		_helper->cancel();
	}

	_helper = std::make_shared<Helper>(ptr());

	_helper->start(alarmTime);

	return alarmTime;
}

void Timer::stop()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (_helper)
	{
		_helper->cancel();
		_helper.reset();
	}
}
