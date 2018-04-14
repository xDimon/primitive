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
// File created on: 2017.09.20

// Counter.hpp


#pragma once

#include <memory>
#include <string>
#include "../../serialization/SObj.hpp"

class CounterConfig;

class Counter
{
public:
	typedef std::string Id;
	typedef uint32_t Value;
	typedef uint32_t Delta;

	const Id id; // Id счетчика

private:
	std::shared_ptr<const CounterConfig> _config;

	Value _value; // Текущее значение
	bool _changed;

public:
	Counter() = delete;

	Counter(const Counter&) = delete; // Copy-constructor
	Counter& operator=(Counter const&) = delete; // Copy-assignment
	Counter(Counter&&) noexcept = delete; // Move-constructor
	Counter& operator=(Counter&&) noexcept = delete; // Move-assignment

	explicit Counter(const Id& id);

	Counter(const Id& id, Value value);

	virtual ~Counter() = default;

	inline Value value() const
	{
		return _value;
	}

	bool increase(Delta delta);

	bool increaseUpto(Value value);

	bool inDefaultState() const;

	void setChanged(bool isChanged = true);
	inline bool isChanged() const
	{
		return _changed;
	}

	SObj serialize() const;
};
