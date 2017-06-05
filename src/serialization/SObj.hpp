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

// SObj.hpp


#pragma once

#include <map>
#include <algorithm>
#include <functional>
#include "SStr.hpp"
#include "SVal.hpp"
#include "SBool.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"

class SObj : public SVal
{
private:
	std::map<std::string, SVal*> _elements;

public:
	SObj() = default;

	virtual ~SObj()
	{
		while (!_elements.empty())
		{
			auto i = _elements.begin();
			auto value = const_cast<SVal*>(i->second);
			_elements.erase(i);
			delete value;
		}
	};

	SObj(SObj&& tmp)
	{
		_elements.swap(tmp._elements);
	}

	SObj& operator=(SObj&& tmp)
	{
		_elements.swap(tmp._elements);
		return *this;
	}

	void insert(const std::string& key, SVal* value)
	{
		auto i = _elements.find(key);
		if (i != _elements.end())
		{
			auto value = const_cast<SVal*>(i->second);
			_elements.erase(i);
			delete value;
		}
		_elements.emplace(key, value);
	}

	void insert(const std::string key, SVal& value)
	{
		insert(key, &value);
	}

	SVal* get(const std::string& key) const
	{
		auto i = _elements.find(key);
		if (i == _elements.end())
		{
			return nullptr;
		}
		return i->second;
	}
	SVal* get(SStr* key) const
	{
		return get(key->value());
	}
	SVal* get(SStr& key) const
	{
		return get(key);
	}

	void forEach(std::function<void(const std::pair<const std::string, const SVal*>&)> handler) const
	{
		std::for_each(_elements.cbegin(), _elements.cend(), handler);
	}

	void lookup(const std::string& key, bool &value) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		auto elementValue = dynamic_cast<SBool *>(element);
		if (!elementValue)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't boolean");
		}
		value = elementValue->value();
	}

	void lookup(const char *key, double &value) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		auto elementValue = dynamic_cast<const SFloat *>(element);
		if (!elementValue)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't numeric");
		}
		value = elementValue->value();
	}

	void lookup(const char *key, int64_t &value) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		auto elementValue = dynamic_cast<const SInt *>(element);
		if (!elementValue)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't integer");
		}
		value = elementValue->value();
	}

	void lookup(const std::string& key, std::string &value) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		auto elementValue = dynamic_cast<SStr *>(element);
		if (!elementValue)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't string");
		}
		value = elementValue->value();
	}
};
