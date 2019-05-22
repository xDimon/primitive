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
// File created on: 2017.07.02

// ApplicationFactory.cpp


#include <iostream>
#include "ApplicationFactory.hpp"

Dummy ApplicationFactory::reg(const std::string& type, std::shared_ptr<Application>(* creator)(const Setting&)) noexcept
{
	auto& factory = getInstance();

	auto i = factory._creators.find(type);
	if (i != factory._creators.end())
	{
		std::cerr << "Internal error: Attempt to register application with the same type (" << type << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

	factory._creators.emplace(type, creator);

	return Dummy{};
}

std::shared_ptr<Application> ApplicationFactory::create(const Setting& setting)
{
	std::string type;
	if (!setting.lookupValue("type", type) || type.empty())
	{
		throw std::runtime_error("Type undefined");
	}

	auto& factory = getInstance();

	auto i = factory._creators.find(type);
	if (i == factory._creators.end())
	{
		throw std::runtime_error("Unknown type ('" + type + "')");
	}

	return std::move(i->second(setting));
}
