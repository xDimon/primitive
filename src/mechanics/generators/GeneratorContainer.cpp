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
// File created on: 2017.09.21

// GeneratorContainer.cpp


#include "GeneratorContainer.hpp"

GeneratorContainer::GeneratorContainer()
: _changed(false)
{
}

void GeneratorContainer::push(const Generator::Id& id, Time::Timestamp nextTick)
{
	auto i = _counters.find(id);
	if (i == _counters.end())
	{
		auto limit = _counters.emplace(
			id,
			std::make_shared<Generator>(id, nextTick)
		).first->second;
	}
}

std::shared_ptr<Generator> GeneratorContainer::get(const Generator::Id& id)
{
	std::shared_ptr<Generator> counter;
	auto i = _counters.find(id);
	if (i == _counters.end())
	{
		counter = _counters.emplace(
			id,
			std::make_shared<Generator>(id)
		).first->second;
	}
	else
	{
		counter = i->second;
	}
	return counter;
}

void GeneratorContainer::forEach(const std::function<void(const std::shared_ptr<Generator>&)>& handler)
{
	for (auto i : _counters)
	{
		handler(i.second);
	}
}
