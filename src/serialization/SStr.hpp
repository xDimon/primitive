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
// File created on: 2017.04.07

// SStr.hpp


#pragma once


#include "SVal.hpp"

#include <sstream>

class SStr : public SVal
{
private:
	std::string _value;

public:
	SStr() = default;

	SStr(std::string value)
	: _value(value)
	{}

	SStr(SStr&& tmp)
	{
		_value.swap(tmp._value);
	}

	SStr& operator=(SStr&& tmp)
	{
		_value.swap(tmp._value);
		return *this;
	}

	void insert(uint32_t symbol)
	{
		_value.push_back(symbol);
	}

	const std::string& value() const
	{
		return _value;
	}

	struct Cmp
	{
		bool operator()(const SStr* left, const SStr* right)
		{
			return left->value() < right->value();
		}
	};

	virtual operator std::string() const
	{
		return _value;
	};
	virtual operator int() const
	{
		int64_t intVal;
		std::istringstream iss(_value);
		iss >> intVal;
		return intVal;
	};
	virtual operator double() const
	{
		double fltVal;
		std::istringstream iss(_value);
		iss >> fltVal;
		return fltVal;
	};
	virtual operator bool() const
	{
		if (_value.empty())
		{
			return false;
		}
		{
			double fltVal;
			std::istringstream iss(_value);
			iss >> fltVal;
			if (fltVal)
			{
				return true;
			}
		}
		{
			std::istringstream iss(_value);
			for (;;)
			{
				auto c = iss.get();
				if (c == -1)
				{
					return false;
				}
				if (c > ' ')
				{
					return true;
				}
			}
		}
	};
};
