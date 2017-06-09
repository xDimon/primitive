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
// File created on: 2017.05.10

// SBinary.hpp


#pragma once


class SBinary : public SVal
{
private:
	std::string _value;

public:
	SBinary() = default;

	SBinary(std::string& value)
	: _value(std::move(value))
	{
	}

	SBinary(SBinary&& tmp)
	{
		_value.swap(tmp._value);
	}

	SBinary& operator=(SBinary&& tmp)
	{
		_value.swap(tmp._value);
		return *this;
	}

	const std::string& value() const
	{
		return _value;
	}

	virtual operator std::string() const
	{
		std::ostringstream oss;
		oss << "[";
		for (size_t i = 0; i < _value.size(); ++i)
		{
			oss << std::hex << _value[i];
		}
		oss << "]";
		return std::move(oss.str());
	};
	virtual operator int() const
	{
		return _value.size();
	};
	virtual operator double() const
	{
		return _value.size();
	};
	virtual operator bool() const
	{
		return !_value.empty();
	};
};
