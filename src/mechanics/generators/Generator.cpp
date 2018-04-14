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
// File created on: 2017.09.21

// Generator.cpp


#include "Generator.hpp"
#include "GeneratorConfig.hpp"
#include "GeneratorManager.hpp"

Generator::Generator(
	const Id& id
)
: id(id)
, _config(GeneratorManager::get(id))
, _nextTick(0)
, _changed(false)
{
	if (!_config)
	{
		throw std::runtime_error("Not found config for generator '" + id + "'");
	}
}

Generator::Generator(
	const Id& id,
	Time::Timestamp nextTick
)
: Generator(id)
{
	_nextTick = nextTick;
}

bool Generator::inDefaultState() const
{
	return _nextTick == 0;
}

void Generator::setChanged(bool isChanged)
{
	if (_changed == isChanged)
	{
		return;
	}

	_changed = isChanged;
}

bool Generator::start()
{
	std::lock_guard<std::mutex> lockGuard(_mutex);

	if (_nextTick)
	{
		return false;
	}

	if (_config->period == 0)
	{
		return false;
	}

	_nextTick = Time::now() + _config->period;

	setChanged();

	return true;
}

bool Generator::stop()
{
	std::lock_guard<std::mutex> lockGuard(_mutex);

	if (!_nextTick)
	{
		return false;
	}

	_nextTick = 0;

	setChanged();

	return true;
}

bool Generator::tick()
{
	std::lock_guard<std::mutex> lockGuard(_mutex);

	if (Time::now() < _nextTick)
	{
		return false;
	}

	if (_config->period == 0)
	{
		return false;
	}

	size_t ticks = 0;

	while (_nextTick && _nextTick <= Time::now())
	{
		_nextTick += _config->period;
		ticks++;

		// TODO Здесь что-то делаем на тик генератора
	}

	setChanged();

	return true;
}

SObj Generator::serialize() const
{
	SObj data;

	data.emplace("id", id); // Id генератора
	data.emplace("nextTick", _nextTick); // Следующее срабатывание
	data.emplace("period", _config->period); // Периодичность срабатывания

	return std::move(data);
}
