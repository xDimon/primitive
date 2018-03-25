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

// LimitContainer.hpp


#pragma once


#include <memory>
#include <unordered_map>
#include <functional>
#include "Limit.hpp"
#include "../../utils/hash/SipHash.hpp"
#include "../../utils/Shareable.hpp"

class LimitContainer
: public Shareable<LimitContainer>
, public std::unordered_map<std::tuple<Limit::Id, Limit::Clarifier>, std::shared_ptr<Limit>, Limit::KeyHash>
{
private:
	bool _changed;

public:
	LimitContainer(const LimitContainer&) = delete; // Copy-constructor
	LimitContainer& operator=(LimitContainer const&) = delete; // Copy-assignment
	LimitContainer(LimitContainer&&) = delete; // Move-constructor
	LimitContainer& operator=(LimitContainer&&) = delete; // Move-assignment

	LimitContainer();
	~LimitContainer() = default;

	void push(
		const Limit::Id& id,
		const Limit::Clarifier& clarifier,
		Limit::Value count,
		Time::Timestamp expire
	);

	std::shared_ptr<Limit> get(
		const Limit::Id& id,
		const Limit::Clarifier& clarifier
	);

	void setChanged(bool isChanged = true)
	{
		_changed = isChanged;
	}
	inline bool isChanged() const
	{
		return _changed;
	}
};
