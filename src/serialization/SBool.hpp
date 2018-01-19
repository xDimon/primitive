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
// File created on: 2017.04.08

// SBool.hpp


#pragma once

#include "SVal.hpp"

class SBool: public SVal
{
public:
	typedef bool type;

private:
	bool _value;

public:
	SBool(bool value): _value(value) {};

	SBool* clone() const override
	{
		return new SBool(_value);
	}

	bool value() const
	{
		return _value;
	}

	virtual SBool& operator=(SBool const& tmp)
	{
		_value = tmp._value;
		return *this;
	}

	operator std::string() const override
	{
		return std::string(_value ? "true" : "false");
	};
	operator int() const override
	{
		return _value ? 1 : 0;
	};
	operator double() const override
	{
		return _value ? 1 : 0;
	};
	operator bool() const override
	{
		return _value;
	};
};
