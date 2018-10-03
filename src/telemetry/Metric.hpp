// Copyright © 2017-2018 Dmitriy Khaustov
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

// Metric.hpp


#pragma once


#include <cstdint>
#include <chrono>
#include <string>
#include <map>
#include <deque>
#include <mutex>

class Metric
{
public:
	typedef double type;

private:
	const std::string _name;
	double _alertMin;
	double _alertMax;
	double _min;
	double _max;
	size_t _count;
	std::chrono::steady_clock::duration _ttl;
	std::chrono::steady_clock::duration _frame;

	std::mutex _mutex;
	std::deque<std::tuple<int64_t, std::chrono::steady_clock::time_point, double>> _points;

public:
	Metric() = delete;
	Metric(const Metric&) = delete;
	void operator=(Metric const&) = delete;
	Metric(Metric&&) = delete;
	Metric& operator=(Metric&&) = delete;

	Metric(
		std::string name,
		std::chrono::steady_clock::duration ttl,
		std::chrono::steady_clock::duration frame = std::chrono::milliseconds(100),
		type alertMin = std::numeric_limits<type>::min(),
		type alertMax = std::numeric_limits<type>::max(),
		type min = std::numeric_limits<type>::min(),
		type max = std::numeric_limits<type>::max()
	);
	Metric(
		std::string name,
		size_t count,
		type alertMin = std::numeric_limits<type>::min(),
		type alertMax = std::numeric_limits<type>::max(),
		type min = std::numeric_limits<type>::min(),
		type max = std::numeric_limits<type>::max()
	);
	virtual ~Metric() = default;

	const std::string& name() const
	{
		return _name;
	}

	void setValue(type value, std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now());
	void addValue(type value = 1, std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now());

	type sum(std::chrono::steady_clock::duration interval);
	type sum(size_t count);

	type avg(size_t count);
	type avg(std::chrono::steady_clock::duration interval);

	type avgPerSec(std::chrono::steady_clock::duration interval);
};
