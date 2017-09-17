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
// File created on: 2017.09.17

// LimitManager.cpp


#include "LimitManager.hpp"

void LimitManager::push(
	const Limit::Id& id,
	Limit::Type type,
	Limit::Value start,
	Limit::Value max,
	uint32_t offset,
	uint32_t duration
)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	lm._configs.erase(id);
	lm._configs.emplace(
		id,
		std::make_shared<LimitConfig>(
			id,
			type,
			start,
			max,
			offset,
			duration
		)
	);
}

std::shared_ptr<const LimitConfig> LimitManager::get(const Limit::Id& id)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	auto i = lm._configs.find(id);
	return (i != lm._configs.end()) ? i->second : nullptr;
}

void LimitManager::remove(const Limit::Id& id)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	lm._configs.erase(id);
}

void LimitManager::forEach(const std::function<void(const std::shared_ptr<const LimitConfig>&)>& handler)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	for (auto i : lm._configs)
	{
		handler(i.second);
	}
}
