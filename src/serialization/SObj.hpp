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

// SObj.hpp


#pragma once

#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <cstddef>
#include "SVal.hpp"
#include "SArr.hpp"

class SObj final : public SBase, public std::map<std::string, SVal>
{
public:
	// Default-constructor
	SObj() = default;

	// Copy-constructor
	SObj(const SObj& that)
	: std::map<std::string, SVal>(static_cast<const std::map<std::string, SVal>&>(that))
	{
	}

	// Move-constructor
	SObj(SObj&& that) noexcept
	: std::map<std::string, SVal>(std::move(static_cast<std::map<std::string, SVal>&>(that)))
	{
	}

	// Copy-assignment
	SObj& operator=(const SObj& that)
	{
		static_cast<std::map<std::string, SVal>&>(*this) = static_cast<const std::map<std::string, SVal>&>(that);
		return *this;
	}

	// Move-assignment
	SObj& operator=(SObj&& that) noexcept
	{
		static_cast<std::map<std::string, SVal>&>(*this) = std::move(static_cast<std::map<std::string, SVal>&>(that));
		return *this;
	}

	const SVal& get(const std::string& key, bool strict = false) const
	{
		auto i = find(key);
		if (i == end())
		{
			if (strict)
			{
				throw std::runtime_error("Field '" + key + "' not found");
			}
			static const SVal undefined;
			return undefined;
		}
		return i->second;
	}

	SVal extract(const std::string& key)
	{
		SVal ret;
		auto i = find(key);
		if (i == end())
		{
			return SVal();
		}
		ret = std::move(i->second);
		erase(i);
		return ret;
	}

	template<typename T>
	void lookup(const std::string& key, T &value, bool strict = false) const
	{
		const auto& element = get(key);
		if (element.isUndefined())
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		value = (T)(element);
	}

	template <typename T>
	inline void trylookup(const std::string& key, T &value) const noexcept
	{
		try	{ lookup(key, value, false); } catch (...) { value = T(); }
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		return size();
	}

	operator std::string() const
	{
		std::ostringstream oss;
		oss << "[object#" << this << "(" << size() << ")]";
		return std::move(oss.str());
	}
};
