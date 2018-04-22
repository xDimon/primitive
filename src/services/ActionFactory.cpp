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
// File created on: 2017.05.31

// ActionFactory.cpp


#include "ActionFactory.hpp"

Dummy ActionFactory::reg(
	const std::string& name,
	std::shared_ptr<Action>(* creator)(
		const std::shared_ptr<ServicePart>&,
		const std::shared_ptr<Context>&,
		const SVal&
	)
) noexcept
{
	auto& factory = getInstance();

	auto i = factory._creators.find(name);
	if (i != factory._creators.end())
	{
		std::cerr << "Internal error: Attepmt to register action with the same name (" << name << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
	factory._creators.emplace(name, creator);
	return Dummy{};
}

std::shared_ptr<Action> ActionFactory::create(
	const std::string& name,
	const std::shared_ptr<ServicePart>& servicePart,
	const std::shared_ptr<Context>& context,
	const SVal& input
)
{
	auto& factory = getInstance();

	auto i = factory._creators.find(name);
	if (i == factory._creators.end())
	{
		return nullptr;
	}
	return i->second(servicePart, context, input);
}
