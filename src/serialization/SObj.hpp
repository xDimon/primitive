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

// SObj.hpp


#pragma once

#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <cstddef>
#include "SVal.hpp"

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

	bool has(const std::string& key) const
	{
		return find(key) != end();
	}

	template <typename T>
	bool hasOf(const std::string& key) const
	{
		auto i = find(key);
		return i != end() && i->second.is<T>();
	}

	const SVal& get(const std::string& key) const
	{
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		return i->second;
	}

	SVal& get(const std::string& key)
	{
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		return i->second;
	}

	template <typename T>
	T& getAs(const std::string& key)
	{
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!i->second.is<T>())
		{
			if (i->second.isUndefined())
			{
				throw std::runtime_error("Field '" + key + "' undefined");
			}
			throw std::runtime_error("Field '" + key + "' has other type");
		}
		return i->second.as<T>();
	}

	template <typename T>
	const T& getAs(const std::string& key) const
	{
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!i->second.is<T>())
		{
			if (i->second.isUndefined())
			{
				throw std::runtime_error("Field '" + key + "' undefined");
			}
			throw std::runtime_error("Field '" + key + "' has other type");
		}
		return i->second.as<T>();
	}

	SVal extract(const std::string& key)
	{
		SVal ret;
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' undefined");
		}
		ret = std::move(i->second);
		erase(i);
		return std::move(ret);
	}

	template <typename T>
	T extractAs(const std::string& key)
	{
		auto i = find(key);
		if (i == end())
		{
			throw std::runtime_error("Field '" + key + "' undefined");
		}
		if (!i->second.is<T>())
		{
			if (i->second.isUndefined())
			{
				throw std::runtime_error("Field '" + key + "' undefined");
			}
			throw std::runtime_error("Field '" + key + "' has other type");
		}
		auto ret = std::move(i->second);
		erase(i);
		return std::move(ret.as<T>());
	}

	template<typename T>
	void lookup(const std::string& key, T &value) const
	{
		value = (T)get(key);
	}

	template <typename T>
	inline void trylookup(const std::string& key, T &value) const noexcept
	{
		try	{ lookup(key, value); } catch (...) { value = T(); }
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
