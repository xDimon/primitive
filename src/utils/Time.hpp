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

class Time
{
public:
	static auto httpDate(std::time_t* ts_ = nullptr)
	{
		std::time_t ts;
		if (ts_)
		{
			ts = *ts_;
		}
		else
		{
			ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		}
		std::tm tm;
		::localtime_r(&ts, &tm);
		std::ostringstream ss;
		ss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S %Z");
		return ss.str();
	}
};
