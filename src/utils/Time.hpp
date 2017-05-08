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
// File created on: 2017.04.02

// Time.hpp


#pragma once

#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace Time
{

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
	ETERNITY,
	_COUNT
};

Timestamp interval(Time::Interval interval, size_t number = 0);

Timestamp trim(Timestamp value, Time::Interval quant);

std::string httpDate(std::time_t* ts_ = nullptr);

};
