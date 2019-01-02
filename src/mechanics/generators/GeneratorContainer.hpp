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

// GeneratorContainer.hpp


#pragma once


#include <unordered_map>
#include "Generator.hpp"
#include "../../utils/hash/SipHash.hpp"

class GeneratorContainer
{
private:
	struct KeyHash
	{
		size_t operator()(const Generator::Id& id) const noexcept
		{
			static const char hidSeed[16]{};
			SipHash hash(hidSeed);
			hash(id);
			return hash.computeHash();
		}
	};

	std::unordered_map<
		Generator::Id,
		std::shared_ptr<Generator>,
		GeneratorContainer::KeyHash
	> _counters;

	bool _changed;

public:
	GeneratorContainer(const GeneratorContainer&) = delete; // Copy-constructor
	GeneratorContainer& operator=(GeneratorContainer const&) = delete; // Copy-assignment
	GeneratorContainer(GeneratorContainer&&) noexcept = delete; // Move-constructor
	GeneratorContainer& operator=(GeneratorContainer&&) noexcept = delete; // Move-assignment

	GeneratorContainer();
	~GeneratorContainer() = default;

	void push(
		const Generator::Id& id,
		Time::Timestamp nextTick
	);

	std::shared_ptr<Generator> get(
		const Generator::Id& id
	);

	void forEach(const std::function<
		void(const std::shared_ptr<Generator>&)
	>& handler);

	auto& operator*()
	{
		return _counters;
	}

	void setChanged(bool isChanged = true)
	{
		_changed = isChanged;
	}
	bool isChanged() const
	{
		return _changed;
	}
};
