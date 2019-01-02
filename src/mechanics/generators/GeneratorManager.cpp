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

// GeneratorManager.cpp


#include "GeneratorManager.hpp"

void GeneratorManager::push(
	const Generator::Id& id,
	Generator::Period period
)
{
	auto& gm = getInstance();

	std::lock_guard<std::mutex> lockGuard(gm._mutex);

	gm._configs.erase(id);
	gm._configs.emplace(
		id,
		std::make_shared<GeneratorConfig>(
			id,
			period
		)
	);
}

std::shared_ptr<const GeneratorConfig> GeneratorManager::get(const Generator::Id& id)
{
	auto& gm = getInstance();

	std::lock_guard<std::mutex> lockGuard(gm._mutex);

	auto i = gm._configs.find(id);
	return (i != gm._configs.end()) ? i->second : nullptr;
}
