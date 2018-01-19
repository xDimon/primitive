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

// SArr.hpp


#pragma once

#include <sstream>
#include <vector>
#include <functional>
#include <algorithm>
#include <stddef.h>
#include "SStr.hpp"
#include "SVal.hpp"
#include "SBool.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SNull.hpp"

class SArr: public SVal, public std::vector<SVal*>
{
public:
	SArr() = default;

	~SArr() override
	{
		for (auto element : *this)
		{
			delete element;
		}
	};

	SArr(SArr&& tmp)
	{
		swap(tmp);
	}

	SArr& operator=(SArr&& tmp)
	{
		swap(tmp);
		return *this;
	}

	void insert(SVal* value)
	{
		emplace_back(value);
	}

	void insert(const SVal* value)
	{
		emplace_back(const_cast<SVal*>(value));
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, void>::type
	insert(T value)
	{
		insert(new SInt(value));
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, void>::type
	insert(T value)
	{
		insert(new SFloat(value));
	}

//	void insert(double value)
//	{
//		insert(new SFloat(value));
//	}

	void insert(bool value)
	{
		insert(new SBool(value));
	}

	void insert(const char* value)
	{
		insert(new SStr(value));
	}

	void insert(const std::string& value)
	{
		insert(new SStr(value));
	}

	void insert(nullptr_t)
	{
		insert(new SNull());
	}

	const SVal* operator[](size_t index) const
	{
		return index < size() ? std::vector<SVal*>::operator[](index) : nullptr;
	}

	SVal* operator[](size_t index)
	{
		return index < size() ? std::vector<SVal*>::operator[](index) : nullptr;
	}

	template< template<typename, typename> class C, typename E, typename A>
	void fill(C<E, A> &container) const noexcept
	{
		for (auto& element : *this)
		{
			container.emplace_back(element);
		}
	}

	operator std::string() const override
	{
		std::ostringstream oss;
		oss << "[array#" << this << "(" << size() << ")]";
		return std::move(oss.str());
	}

	operator int() const override
	{
		return size();
	}

	operator double() const override
	{
		return size();
	}

	operator bool() const override
	{
		return !empty();
	}

	SArr* clone() const override
	{
		auto copy = std::make_unique<SArr>();
		for (const auto& element : *this)
		{
			copy->emplace_back(element->clone());
		}
		return copy.release();
	}
};
