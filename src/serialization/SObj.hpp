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
#include <stddef.h>
#include "SStr.hpp"
#include "SVal.hpp"
#include "SBool.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SNull.hpp"

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

	void insert(const std::string key, SInt::type value)
	{
		insert(key, new SInt(value));
	}

	void insert(const std::string key, SFloat::type value)
	{
		insert(key, new SFloat(value));
	}

	void insert(const std::string key, const std::string value)
	{
		insert(key, new SStr(value));
	}

	void insert(const std::string key, bool value)
	{
		insert(key, new SBool(value));
	}

	void insert(const std::string key, nullptr_t)
	{
		insert(key, new SNull());
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

	void lookup(const char *key, int64_t &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		if (!dynamic_cast<const SInt *>(element) && strict)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't integer");
		}
		value = element->operator int();
	}

	void lookup(const std::string& key, bool &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		if (!dynamic_cast<SBool *>(element) && strict)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't boolean");
		}
		value = static_cast<bool>(element);
	}

	void lookup(const char *key, double &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		if (!dynamic_cast<const SFloat *>(element) && strict)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't numeric");
		}
		value = element->operator double();
	}

	void lookup(const std::string& key, std::string &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' not found");
		}
		if (!dynamic_cast<SStr *>(element) && strict)
		{
			throw std::runtime_error(std::string() + "Field '" + key + "' isn't string");
		}
		value = element->operator std::string();
	}

	virtual operator std::string() const
	{
		std::ostringstream oss;
		oss << "[object#" << this << "(" << _elements.size() << ")]";
		return std::move(oss.str());
	};
	virtual operator int() const
	{
		return _elements.size();
	};
	virtual operator double() const
	{
		return _elements.size();
	};
	virtual operator bool() const
	{
		return !_elements.empty();
	};
};
