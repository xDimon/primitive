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
// File created on: 2017.04.07

// SStr.hpp


#pragma once

#include "SBase.hpp"

#include <string>
#include <sstream>

class SStr : public SBase
{
public:
	typedef std::string type;

private:
	type _value;

public:
	// Default-constructor
	SStr() = default;

	SStr(const std::string& value)
	: _value(value)
	{
	}

	SStr(std::string&& value)
	: _value(std::move(value))
	{
	}

	// Copy-constructor
	SStr(const SStr& that)
	: _value(that._value)
	{
	}

	// Move-constructor
	SStr(SStr&& that) noexcept
	: _value(std::move(that._value))
	{
	}

	// Copy-assignment
	virtual SStr& operator=(SStr const& that)
	{
		_value = that._value;
		return *this;
	}

	// Move-assignment
	SStr& operator=(SStr&& that) noexcept
	{
		_value = std::move(that._value);
		return *this;
	}

	// Compare
	bool operator<(const SStr& other)
	{
		return _value < other._value;
	}

	void insert(uint32_t symbol)
	{
		_value.push_back(symbol);
	}

	const std::string& value() const
	{
		return _value;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		T ret = 0;
		std::istringstream(_value).operator>>(ret);
		return ret;
	}

	operator std::string() const
	{
		return _value;
	}
};
