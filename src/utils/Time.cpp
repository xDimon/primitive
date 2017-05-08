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

// Time.cpp


#include "Time.hpp"

namespace Time
{

std::string httpDate(std::time_t* ts_)
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

Timestamp interval(Time::Interval interval, size_t number)
{
	switch (interval)
	{
		case Interval::ZERO:
			return (number?number:1) * 0;

		case Interval::SECOND:
			return (number?number:1) * 1;

		case Interval::MINUTE:
			return (number?number:1) * 60;

		case Interval::HOUR:
			return (number?number:1) * 3600;

		case Interval::DAY:
			return (number?number:1) * 86400;

		case Interval::WEEK:
			return (number?number:1) * 86400 * 7;

		case Interval::MONTH:
		{
			if (number)
			{
				time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				tm tm;

				localtime_r(&ts, &tm);

				tm.tm_mon += number;

				time_t ts2 = mktime(&tm);
				ts2 += tm.tm_gmtoff;
				ts2 -= ts;
				return ts2;
			}
			else
			{
				return 86400 * 30;
			}
		}

		case Interval::YEAR:
		{
			if (number)
			{
				time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				tm tm;

				localtime_r(&ts, &tm);

				tm.tm_year += number;

				time_t ts2 = mktime(&tm);
				ts2 += tm.tm_gmtoff;
				ts2 -= ts;
				return ts2;
			}
			else
			{
				return 86400 * 365;
			}
		}

		case Interval::ETERNITY:
			return std::numeric_limits<Timestamp>::max();

		default: throw;
	}
}

Timestamp trim(Timestamp timestamp, Interval quant)
{
	if (quant == Interval::WEEK)
	{
		timestamp -= 4 * 86400;
	}

	tm tm;
	localtime_r(&timestamp, &tm);

	if (quant == Interval::ZERO) goto done;
	if (quant == Interval::SECOND) goto done;

	tm.tm_sec = 0;

	if (quant == Interval::MINUTE) goto done;

	tm.tm_min = 0;

	if (quant == Interval::HOUR) goto done;

	tm.tm_hour = 0;

	if (quant == Interval::DAY) goto done;
	if (quant == Interval::WEEK) goto done;

	tm.tm_mday = 1;

	if (quant == Interval::MONTH) goto done;

	tm.tm_mon = 0;

	if (quant == Interval::YEAR) goto done;

	tm.tm_year = 70;

	if (quant == Interval::ETERNITY) goto done;

	done:
	tm.tm_gmtoff = 0;

	time_t ts2 = mktime(&tm);

	return ts2;
}

}
