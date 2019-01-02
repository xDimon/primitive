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
// File created on: 2017.08.14

// Metric.cpp


#include <memory>
#include "Metric.hpp"
#include "../log/Log.hpp"

Metric::Metric(
	std::string name,
	std::chrono::steady_clock::duration ttl,
	std::chrono::steady_clock::duration frame,
	Metric::type alertMin,
	Metric::type alertMax,
	Metric::type min,
	Metric::type max
)
: _name(std::move(name))
, _alertMin(alertMin)
, _alertMax(alertMax)
, _min(min)
, _max(max)
, _count(0)
, _ttl(ttl)
, _frame(frame)
{
}

Metric::Metric(
	std::string name,
	size_t count,
	Metric::type alertMin,
	Metric::type alertMax,
	Metric::type min,
	Metric::type max
)
: _name(std::move(name))
, _alertMin(alertMin)
, _alertMax(alertMax)
, _min(min)
, _max(max)
, _count(count)
, _ttl(std::chrono::steady_clock::duration::zero())
, _frame(std::chrono::steady_clock::duration::max())
{
}

void Metric::setValue(Metric::type value, std::chrono::steady_clock::time_point time)
{
	std::lock_guard<std::mutex> lockGuard(_mutex);

	int64_t frame = time.time_since_epoch().count() / _frame.count();

	// Wipe by count
	if (_count > 0)
	{
		while (_points.size() > _count)
		{
			_points.pop_back();
		}
	}

	// Wipe by ttl
	if (_ttl != std::chrono::steady_clock::duration::zero())
	{
		auto expireTime = time - _ttl;

		while (!_points.empty() && std::get<1>(_points.back()) < expireTime)
		{
			_points.pop_back();
		}
	}

	if (_points.empty() || std::get<0>(_points.front()) != frame)
	{
		_points.emplace_front(frame, time, value);
	}
	else
	{
		std::get<1>(_points.front()) = time;
		std::get<2>(_points.front()) = value;
	}

//	Log("SysInfo").info("Metric '%s': %0.15f", _name.c_str(), value);
}

void Metric::addValue(Metric::type value, std::chrono::steady_clock::time_point time)
{
	std::lock_guard<std::mutex> lockGuard(_mutex);

	int64_t frame = time.time_since_epoch().count() / _frame.count();

	// Wipe by count
	if (_count > 0)
	{
		while (_points.size() > _count)
		{
			_points.pop_back();
		}
	}

	// Wipe by ttl
	if (_ttl != std::chrono::steady_clock::duration::zero())
	{
		auto expireTime = time - _ttl;

		while (!_points.empty() && std::get<1>(_points.back()) < expireTime)
		{
			_points.pop_back();
		}
	}

	if (_points.empty() || std::get<0>(_points.front()) != frame)
	{
		_points.emplace_front(frame, time, value);
	}
	else
	{
		std::get<1>(_points.front()) = time;
		std::get<2>(_points.front()) += value;
	}

//	Log("SysInfo").info("Metric '%s': %0.15f", _name.c_str(), value);
}

Metric::type Metric::avg(std::chrono::steady_clock::duration interval)
{
	if (_points.empty())
	{
		return 0;
	}

	auto earliestTime = std::chrono::steady_clock::now() - interval;

	Metric::type sum = 0;
	std::chrono::steady_clock::time_point endTime = std::get<1>(_points.front());
	std::chrono::steady_clock::time_point beginTime = endTime;

	int i = 0;
	for (const auto& point : _points)
	{
		beginTime = std::get<1>(point);
		if (beginTime < earliestTime)
		{
			break;
		}
		sum += std::get<2>(point);
		i++;
	}

	return sum / i;
}

Metric::type Metric::avgPerSec(std::chrono::steady_clock::duration interval)
{
	if (_points.empty())
	{
		return 0;
	}

	auto earliestTime = std::chrono::steady_clock::now() - interval;

	Metric::type sum = 0;
	std::chrono::steady_clock::time_point endTime = std::get<1>(_points.front());
	std::chrono::steady_clock::time_point beginTime = endTime;

	for (const auto& point : _points)
	{
		beginTime = std::get<1>(point);
		if (beginTime < earliestTime)
		{
			break;
		}
		sum += std::get<2>(point);
	}

	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(endTime - beginTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());

	return (timeSpent > 0) ? (sum / timeSpent) : 0;
}

Metric::type Metric::avg(size_t count)
{
	if (_points.empty())
	{
		return 0;
	}

	Metric::type sum = 0;

	size_t i = 0;
	for (const auto& point : _points)
	{
		if (i++ > count)
		{
			break;
		}
		sum += std::get<2>(point);
	}

	return sum / i;
}

Metric::type Metric::sum(std::chrono::steady_clock::duration interval)
{
	if (_points.empty())
	{
		return 0;
	}

	auto earliestTime = std::chrono::steady_clock::now() - interval;

	Metric::type sum = 0;
	std::chrono::steady_clock::time_point endTime = std::get<1>(_points.front());
	std::chrono::steady_clock::time_point beginTime = endTime;

	for (const auto& point : _points)
	{
		beginTime = std::get<1>(point);
		if (beginTime < earliestTime)
		{
			break;
		}
		sum += std::get<2>(point);
	}

	return sum;
}

Metric::type Metric::sum(size_t count)
{
	if (_points.empty())
	{
		return 0;
	}

	Metric::type sum = 0;

	size_t i = 0;
	for (const auto& point : _points)
	{
		if (i++ > count)
		{
			break;
		}
		sum += std::get<2>(point);
	}

	return sum;
}
