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

class SBinary final: public SBase
{
public:
	typedef std::vector<char> type;

private:
	type _value;

public:
	// Default-constructor
	SBinary() = default;

	SBinary(const void* data_, size_t size)
	{
		auto data = reinterpret_cast<const char*>(data_);
		_value.reserve(size + 1);
		for (size_t i = 0; i < size; i++)
		{
			_value.push_back(data[i]);
		}
	}

	SBinary(std::string&& value)
	{
		_value.reserve(value.length());
		for (size_t i = 0; i < value.length(); i++)
		{
			_value.push_back(value[i]);
		}
	}

	SBinary(const std::string& value)
	{
		_value.reserve(value.length());
		for (size_t i = 0; i < value.length(); i++)
		{
			_value.push_back(value[i]);
		}
	}

	// Copy-constructor
	SBinary(const SBinary& that)
	: _value(that._value)
	{
	}

	// Move-constructor
	SBinary(SBinary&& that) noexcept
	: _value(std::move(that._value))
	{
	}

	// Copy-assignment
	virtual SBinary& operator=(SBinary const& that)
	{
		_value = that._value;
		return *this;
	}

	// Move-assignment
	SBinary& operator=(SBinary&& that) noexcept
	{
		_value = std::move(that._value);
		return *this;
	}

	const auto& value() const
	{
		return _value;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		return _value.size();
	}

	operator std::string() const
	{
		std::ostringstream oss;
		oss << "[";
		for (size_t i = 0; i < _value.size(); ++i)
		{
			oss << std::hex << _value[i];
		}
		oss << "]";
		return std::move(oss.str());
	}
};
