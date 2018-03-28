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

// CounterManager.hpp


#pragma once


#include <unordered_map>
#include <mutex>
#include "CounterConfig.hpp"
#include "../../utils/hash/SipHash.hpp"

class CounterManager
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
		std::shared_ptr<const CounterConfig>,
		KeyHash
	> _configs;

	std::mutex _mutex;

public:
	CounterManager(const CounterManager&) = delete; // Copy-constructor
	CounterManager& operator=(CounterManager const&) = delete; // Copy-assignment
	CounterManager(CounterManager&&) noexcept = delete; // Move-constructor
	CounterManager& operator=(CounterManager&&) noexcept = delete; // Move-assignment

private:
	CounterManager() = default;
	~CounterManager() = default;

	static CounterManager &getInstance()
	{
		static CounterManager instance;
		return instance;
	}

public:
	static void push(
		const Counter::Id& id // Id счетчика
	);

	static std::shared_ptr<const CounterConfig> get(
		const Counter::Id& id
	);
};
