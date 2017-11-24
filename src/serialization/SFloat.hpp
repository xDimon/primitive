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
// File created on: 2017.04.17

// SFloat.hpp


#pragma once

#include "SNum.hpp"
#include <cmath>

class SFloat : public SNum
{
public:
	typedef double_t type;

private:
	type _value;

public:
	SFloat(type value)
	: _value(value)
	{}

	SFloat* clone() const override
	{
		return new SFloat(_value);
	}

	type value() const
	{
		return _value;
	}

	void operator=(SFloat const& that)
	{
		_value = that._value;
	}

	operator std::string() const override
	{
		std::ostringstream oss;
		oss << _value;
		return std::move(oss.str());
	};
	operator int() const override
	{
		return _value;
	};
	operator double() const override
	{
		return _value;
	};
	operator bool() const override
	{
		return _value != 0;
	};
};
