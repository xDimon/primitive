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

// GeneratorManager.hpp


#pragma once


#include <unordered_map>
#include <mutex>
#include "GeneratorConfig.hpp"
#include "../../utils/hash/SipHash.hpp"

class GeneratorManager
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
		std::shared_ptr<const GeneratorConfig>,
		KeyHash
	> _configs;

	std::mutex _mutex;

public:
	GeneratorManager(const GeneratorManager&) = delete; // Copy-constructor
	GeneratorManager& operator=(GeneratorManager const&) = delete; // Copy-assignment
	GeneratorManager(GeneratorManager&&) noexcept = delete; // Move-constructor
	GeneratorManager& operator=(GeneratorManager&&) noexcept = delete; // Move-assignment

private:
	GeneratorManager() = default;
	~GeneratorManager() = default;

	static GeneratorManager &getInstance()
	{
		static GeneratorManager instance;
		return instance;
	}

public:
	static void push(
		const Generator::Id& id,
		Generator::Period period
	);

	static std::shared_ptr<const GeneratorConfig> get(
		const Generator::Id& id
	);
};
