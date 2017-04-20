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

class SObj: public SVal
{
private:
	std::map<const SStr*, SVal*> _elements;

public:
	SObj() = default;

	virtual ~SObj()
	{
		while (!_elements.empty())
		{
			auto i = _elements.begin();
			auto key = const_cast<SStr*>(i->first);
			auto value = const_cast<SVal*>(i->second);
			_elements.erase(i);
			delete key;
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

	void insert(SStr *key, SVal *value)
	{
		auto i = _elements.find(key);
		if (i != _elements.end())
		{
			auto i = _elements.begin();
			auto key = const_cast<SStr*>(i->first);
			auto value = const_cast<SVal*>(i->second);
			_elements.erase(i);
			delete key;
			delete value;
		}
		_elements.emplace(key, value);
	}

	void forEach(std::function<void (const std::pair<const SStr* const, SVal*>&)> handler) const
	{
		std::for_each(_elements.begin(), _elements.end(), handler);
	}
};
