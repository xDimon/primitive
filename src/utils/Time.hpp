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
// File created on: 2017.04.02

// Time.hpp


#pragma once

#include <iostream>
#include <ctime>
#include <chrono>

namespace Time
{

extern std::chrono::system_clock::time_point startTime;

extern std::chrono::steady_clock::time_point stStartTime;

inline int64_t msFromStart()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stStartTime).count();
}

inline int64_t msFromStart(std::chrono::steady_clock::time_point tp)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(tp - stStartTime).count();
}

typedef std::time_t Timestamp;

enum class Interval
{
	ZERO,
	SECOND,
	MINUTE,
	HOUR,
	DAY,
	WEEK,
	MONTH,
	YEAR,
	ETERNITY
};

inline Timestamp now()
{
	return std::time(nullptr);
}

Timestamp interval(Time::Interval interval, size_t number = 0);

Timestamp trim(Timestamp value, Time::Interval quantity);

std::string httpDate(std::time_t* ts = nullptr);

};
