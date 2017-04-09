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
#include "SStr.hpp"
#include "SVal.hpp"

class SObj: public SVal
{
private:
	std::map<SStr, SVal&> _elements;

public:
	SObj()
	{
	};
	virtual ~SObj()
	{
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

	void insert(SStr &key, SVal &value)
	{
		_elements.emplace(std::move(key), value);
	}
};
