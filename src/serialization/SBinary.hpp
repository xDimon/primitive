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
// File created on: 2017.05.10

// SBinary.hpp


#pragma once

#include <iomanip>

class SBinary final: public SBase, public std::vector<char>
{
public:
	// Default-constructor
	SBinary() = default;

	SBinary(const void* data, size_t size)
	: std::vector<char>(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + size)
	{
	}

	SBinary(std::string&& value)
	: std::vector<char>(value.begin(), value.end())
	{
	}

	SBinary(const std::string& value)
	{
		reserve(value.length());
		std::copy(value.begin(), value.end(), begin());
	}

	// Copy-constructor
	SBinary(const SBinary& that)
	: std::vector<char>(that)
	{
	}

	// Move-constructor
	SBinary(SBinary&& that) noexcept
	: std::vector<char>(std::move(that))
	{
	}

	// Copy-assignment
	SBinary& operator=(SBinary const& that)
	{
		dynamic_cast<std::vector<char>&>(*this) = dynamic_cast<const std::vector<char>&>(that);
		return *this;
	}

	// Move-assignment
	SBinary& operator=(SBinary&& that) noexcept
	{
		dynamic_cast<std::vector<char>&>(*this) = std::move(dynamic_cast<const std::vector<char>&>(that));
		return *this;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		return size();
	}

	explicit operator std::string() const
	{
		std::ostringstream oss;
		oss << '[';
		for (char i : *this)
		{
			oss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << i;
		}
		oss << ']';
		return oss.str();
	}
};
