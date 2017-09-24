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

class SArr: public SVal
{
private:
	std::vector<SVal*> _elements;

public:
	SArr() = default;

	~SArr() override
	{
		for (auto element : _elements)
		{
			delete element;
		}
	};

	SArr* clone() const override
	{
		SArr *copy = new SArr();
		forEach([&](const SVal& value){
			copy->insert(value.clone());
		});
		return copy;
	}

	SArr(SArr&& tmp)
	{
		_elements.swap(tmp._elements);
	}

	SArr& operator=(SArr&& tmp)
	{
		_elements.swap(tmp._elements);
		return *this;
	}

	void insert(SVal* value)
	{
		_elements.emplace_back(value);
	}

	void insert(const SVal* value)
	{
		_elements.emplace_back(const_cast<SVal*>(value));
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

	void insert(double value)
	{
		insert(new SFloat(value));
	}

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

	void forEach(std::function<void (const SVal*)> handler) const
	{
		for (auto const& element : _elements)
		{
			handler(element);
		}
	}

	void forEach(std::function<void (const SVal&)> handler) const
	{
		for (auto const& element : _elements)
		{
			handler(*element);
		}
	}

	template< template<typename, typename> class C, typename E, typename A>
	void fill(C<E, A> &container) const noexcept
	{
		forEach([&](const SVal* value){
			container.emplace_back(value);
		});
	}

	operator std::string() const override
	{
		std::ostringstream oss;
		oss << "[array#" << this << "(" << _elements.size() << ")]";
		return std::move(oss.str());
	};
	operator int() const override
	{
		return _elements.size();
	};
	operator double() const override
	{
		return _elements.size();
	};
	operator bool() const override
	{
		return !_elements.empty();
	};
};
