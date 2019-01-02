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
// File created on: 2017.09.20

// Counter.cpp


#include "Counter.hpp"
#include "CounterManager.hpp"

Counter::Counter(const Counter::Id& id, Counter::Value value)
: id(id)
, _config(CounterManager::get(id))
, _value(value)
, _changed(false)
{
	if (!_config)
	{
		throw std::runtime_error("Not found config for config '" + id + "'");
	}

	_changed = false;
}

Counter::Counter(const Counter::Id& id)
: Counter(id, 0)
{
}

bool Counter::increase(Counter::Delta delta)
{
	if (!delta)
	{
		return false;
	}

	_value += delta;

	return false;
}

bool Counter::increaseUpto(Counter::Value value)
{
	if (value <= _value)
	{
		return false;
	}

	return increase(value - _value);
}

bool Counter::inDefaultState() const
{
	return _value == 0;
}

void Counter::setChanged(bool isChanged)
{
	if (_changed == isChanged)
	{
		return;
	}

	_changed = isChanged;
}

SObj Counter::serialize() const
{
	SObj data;

	data.emplace("id", id); // Id
	data.emplace("value", _value); // значение

	return data;
}
