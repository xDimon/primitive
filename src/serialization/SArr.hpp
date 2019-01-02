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

// SArr.hpp


#pragma once

#include <cassert>
#include <sstream>
#include <vector>
#include <functional>
#include <algorithm>
#include <stddef.h>
#include "SVal.hpp"

class SArr final: public SBase, public std::vector<SVal>
{
public:
	// Default-constructor
	SArr() = default;

	// Copy-constructor
	SArr(const SArr& that)
	: std::vector<SVal>(static_cast<const std::vector<SVal>&>(that))
	{
	}

	// Move-constructor
	SArr(SArr&& that) noexcept
	: std::vector<SVal>(std::move(static_cast<std::vector<SVal>&>(that)))
	{
	}

	// Copy-assignment
	SArr& operator=(const SArr& that)
	{
		static_cast<std::vector<SVal>&>(*this) = static_cast<const std::vector<SVal>&>(that);
		return *this;
	}

	// Move-assignment
	SArr& operator=(SArr&& that) noexcept
	{
		static_cast<std::vector<SVal>&>(*this) = std::move(static_cast<std::vector<SVal>&>(that));
		return *this;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		return size();
	}

	operator std::string() const
	{
		std::ostringstream oss;
		oss << "[array#" << this << "(" << size() << ")]";
		return std::move(oss.str());
	}
};
