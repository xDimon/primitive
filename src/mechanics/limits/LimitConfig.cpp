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
// File created on: 2017.09.17

// LimitConfig.cpp


#include "LimitConfig.hpp"

LimitConfig::LimitConfig(
	Limit::Id id, // Id лимита
	Limit::Type type, // Тип поведения
	Limit::Value start, // Начальное значение
	Limit::Value max, // Максимальное значение
	uint32_t offset, // Сдвиг для фиксированных сбросов (Day, Week, Month, Year)
	uint32_t duration // Продолжительность
)
: id(std::move(id))
, type(type)
, start(start)
, max(max)
, offset(
	[offset,this](){
		switch (this->type)
		{
			case Limit::Type::Daily:	return offset % static_cast<uint32_t>(Time::interval(Time::Interval::YEAR, 1));
			case Limit::Type::Weekly:	return offset % static_cast<uint32_t>(Time::interval(Time::Interval::WEEK, 1));
			case Limit::Type::Monthly:	return offset % static_cast<uint32_t>(Time::interval(Time::Interval::MONTH, 1));
			case Limit::Type::Yearly:	return offset % static_cast<uint32_t>(Time::interval(Time::Interval::YEAR, 1));
			default: return 0u;
		}
	}()
)
, duration(duration)
{
}
