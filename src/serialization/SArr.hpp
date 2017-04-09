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

#include <vector>
#include <functional>
#include "SVal.hpp"

class SArr: public SVal
{
private:
	std::vector<std::reference_wrapper<SVal>> _elements;

public:
	SArr()
	{
	};
	virtual ~SArr()
	{
	};

	SArr(SArr&& tmp)
	{
		_elements.swap(tmp._elements);
	}

	SArr& operator=(SArr&& tmp)
	{
		_elements.swap(tmp._elements);
		return *this;
	}

	void insert(SVal& value)
	{
		_elements.emplace_back(value);
	}

};
