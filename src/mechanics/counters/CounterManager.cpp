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

// CounterManager.cpp


#include "CounterManager.hpp"

void CounterManager::push(const Counter::Id& id)
{
	auto& cm = getInstance();

	std::lock_guard<std::mutex> lockGuard(cm._mutex);

	cm._configs.erase(id);
	cm._configs.emplace(
		id,
		std::make_shared<CounterConfig>(
			id
		)
	);
}

std::shared_ptr<const CounterConfig> CounterManager::get(const Counter::Id& id)
{
	auto& cm = getInstance();

	std::lock_guard<std::mutex> lockGuard(cm._mutex);

	auto i = cm._configs.find(id);
	return (i != cm._configs.end()) ? i->second : nullptr;
}
