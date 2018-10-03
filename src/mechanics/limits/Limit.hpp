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
// File created on: 2017.09.17

// Limit.hpp


#pragma once

#include <memory>
#include <string>
#include "../../utils/Time.hpp"
#include "../../serialization/SObj.hpp"

class LimitConfig;

class Limit
{
public:
	typedef std::string Id;
	typedef std::string Clarifier;
	typedef uint32_t Value;
	enum class Type
	{
		None, 		// Нет фиксированного автоматического сброса
		Daily, 		// Сброс ежедневно через offset секунд от полуночи
		Weekly,		// Сброс еженедельно через offset секунд от начала полуночи понедельника
		Monthly, 	// Сброс ежемесячно через offset секунд от полуночи первого числа
		Yearly, 	// Сброс ежегодно через offset секунд от начала года
		Loop,		// Автоматический сброс при превышении максимума (max+1 => start+1)
		Always,		// Сразу достигнут. Вырожденный лимит-блокер
		Never 		// Никогда недостижим. Вырожденный пропускающий лимит
	};

	const Id id; // Id лимита
	const Clarifier clarifier; // Уточнение

private:
	std::shared_ptr<const LimitConfig> _config;

	Value _value; // Текущее значение
	Time::Timestamp _expire; // Время сброса
	bool _changed;

public:
	Limit() = delete;
	Limit(const Limit&) = delete; // Copy-constructor
	void operator=(Limit const&) = delete; // Copy-assignment
	Limit(Limit&&) = delete; // Move-constructor
	Limit& operator=(Limit&&) = delete; // Move-assignment

	Limit(const Id& id, const Clarifier& clarifier);
	Limit(const Id& id, const Clarifier& clarifier, Value value, Time::Timestamp expire);
	virtual ~Limit() = default;

	std::shared_ptr<const LimitConfig> config() const
	{
		return _config;
	}

	inline Value value() const
	{
		return _value;
	}

	inline Time::Timestamp expire() const
	{
		return _expire;
	}
	inline bool isExpired() const
	{
		return _expire > 0 && _expire != Time::interval(Time::Interval::ETERNITY) && _expire < Time::now();
	}

	Value remain() const;
	bool available() const;

	bool change(int32_t delta = 1, Time::Timestamp expire = 0);

	bool reset();
	bool initExpire();
	bool setExpire(Time::Timestamp expire);
 	bool setToMax();

	bool inDefaultState() const;

	void setChanged(bool isChanged = true);
	inline bool isChanged() const
	{
		return _changed;
	}

	SObj* serialize() const;

	static std::string typeToString(Type type);
};
