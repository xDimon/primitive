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
// File created on: 2017.05.31

// ActionFactory.cpp


#include "ActionFactory.hpp"

bool ActionFactory::reg(const std::string& name, std::shared_ptr<Action>(* creator)(Context&, const SVal*))
{
	auto& factory = getInstance();

	auto i = factory._requests.find(name);
	if (i != factory._requests.end())
	{
		throw std::runtime_error(std::string("Attepmt to register action with the same name (") + name + ")");
	}
	factory._requests.emplace(name, creator);
	return true;
}

std::shared_ptr<Action> ActionFactory::create(const std::string& name, Context& context, const SVal* input)
{
	auto& factory = getInstance();

	auto i = factory._requests.find(name);
	if (i == factory._requests.end())
	{
		return nullptr;
	}
	return i->second(context, input);
}
