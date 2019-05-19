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
// File created on: 2017.04.07

// SStr.hpp


#pragma once

#include "SBase.hpp"

#include <string>
#include <sstream>

class SStr final: public SBase, public std::string
{
public:
	// Default-constructor
	SStr() = default;

	SStr(const std::string& value)
	: std::string(value)
	{
	}

	SStr(std::string&& value)
	: std::string(std::forward<std::string>(value))
	{
	}

	// Copy-constructor
	SStr(const SStr& that)
	: std::string(static_cast<const std::string&>(that))
	{
	}

	// Move-constructor
	SStr(SStr&& that) noexcept
	: std::string(std::move(static_cast<std::string&>(that)))
	{
	}

	// Copy-assignment
	SStr& operator=(SStr const& that)
	{
		static_cast<std::string&>(*this) = static_cast<const std::string&>(that);
		return *this;
	}

	// Move-assignment
	SStr& operator=(SStr&& that) noexcept
	{
		static_cast<std::string&>(*this) = std::move(static_cast<const std::string&>(that));
		return *this;
	}

	const std::string& value() const
	{
		return static_cast<const std::string&>(*this);
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		T ret = 0;
		std::istringstream iss(*this);
		iss >> ret;
		return ret;
	}
};
