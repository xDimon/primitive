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

// ServiceFactory.cpp


#include "ServiceFactory.hpp"

bool ServiceFactory::reg(const std::string& name, std::shared_ptr<Service> (* creator)(const Setting&)) noexcept
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

std::shared_ptr<Service> ServiceFactory::create(const Setting& setting)
{
	std::string type;
	try
	{
		setting.lookupValue("type", type);
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		throw std::runtime_error("Bad config or type undefined");
	}

	auto& factory = getInstance();

	auto i = factory._requests.find(type);
	if (i == factory._requests.end())
	{
		throw std::runtime_error(std::string() + "Unknown service type '"+ type +"'");
	}
	return i->second(setting);
}
