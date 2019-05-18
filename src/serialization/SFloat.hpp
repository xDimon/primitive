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
// File created on: 2017.04.17

// SFloat.hpp


#pragma once

#include "SNum.hpp"
#include <cmath>

class SFloat final: public SNum
{
public:
	typedef double_t type;

private:
	type _value;

public:
	SFloat(): _value(0) {};

	SFloat(type value)
	: _value(value)
	{
	}

	type value() const
	{
		return _value;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		return _value;
	}

	explicit operator std::string() const
	{
		return std::to_string(_value);
	}
};
