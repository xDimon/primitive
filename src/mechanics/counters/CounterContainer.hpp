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
// File created on: 2017.09.20

// CounterContainer.hpp


#pragma once


#include <unordered_map>
#include "Counter.hpp"
#include "../../utils/hash/SipHash.hpp"

class CounterContainer
{
private:
	struct KeyHash
	{
		size_t operator()(const Counter::Id& id) const noexcept
		{
			static const char hidSeed[16]{};
			SipHash hash(hidSeed);
			hash(id);
			return hash.computeHash();
		}
	};

	std::unordered_map<
		Counter::Id,
		std::shared_ptr<Counter>,
		CounterContainer::KeyHash
	> _counters;

	bool _changed;

public:
	CounterContainer(const CounterContainer&) = delete; // Copy-constructor
	CounterContainer& operator=(CounterContainer const&) = delete; // Copy-assignment
	CounterContainer(CounterContainer&&) noexcept = delete; // Move-constructor
	CounterContainer& operator=(CounterContainer&&) noexcept = delete; // Move-assignment

	CounterContainer();
	~CounterContainer() = default;

	void push(
		const Counter::Id& id,
		Counter::Value count
	);

	std::shared_ptr<Counter> get(
		const Counter::Id& id
	);

	void forEach(const std::function<
		void(const std::shared_ptr<Counter>&)
	>& handler);

	auto& operator*()
	{
		return _counters;
	}

	void setChanged(bool isChanged = true)
	{
		_changed = isChanged;
	}
	inline bool isChanged() const
	{
		return _changed;
	}
};
