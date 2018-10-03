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
// File created on: 2017.09.17

// LimitContainer.cpp


#include "LimitContainer.hpp"

LimitContainer::LimitContainer()
: _changed(false)
{
}

void LimitContainer::push(
	const Limit::Id& id,
	const Limit::Clarifier& clarifier,
	Limit::Value count,
	Time::Timestamp expire
)
{
	auto i = _limits.find(std::make_tuple(id, clarifier));
	if (i == _limits.end())
	{
		auto limit = _limits.emplace(
			std::make_tuple(id, clarifier),
			std::make_shared<Limit>(
				id,
				clarifier,
				count,
				expire
			)
		).first->second;
		if (limit->isExpired())
		{
			limit->reset();
		}
	}
}

std::shared_ptr<Limit> LimitContainer::get(
	const Limit::Id& id,
	const Limit::Clarifier& clarifier
)
{
	std::shared_ptr<Limit> limit;
	auto i = _limits.find(std::make_tuple(id, clarifier));
	if (i == _limits.end())
	{
		limit = _limits.emplace(
			std::make_tuple(id, clarifier),
			std::make_shared<Limit>(id, clarifier)
		).first->second;
	}
	else
	{
		limit = i->second;
	}
	if (limit->isExpired())
	{
		limit->reset();
	}
	return limit;
}

void LimitContainer::forEach(const std::function<void(const std::shared_ptr<Limit>&)>& handler)
{
	for (auto i : _limits)
	{
		handler(i.second);
	}
}
