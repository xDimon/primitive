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
// File created on: 2017.09.13

// Services.cpp


#include "Services.hpp"

std::shared_ptr<Service> Services::add(const Setting& setting, bool replace)
{
	std::string name;
	if (!setting.lookupValue("name", name) || name.empty())
	{
		throw std::runtime_error("Field name undefined");
	}

	if (!replace)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);
		if (getInstance()._registry.find(name) != getInstance()._registry.end())
		{
			throw std::runtime_error(std::string("Already exists service with the same name ('") + name + "')");
		}
	}

	auto entity = ServiceFactory::create(setting);

	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);

	if (!replace)
	{
		auto i = getInstance()._registry.find(entity->name());
		if (i != getInstance()._registry.end())
		{
			getInstance()._registry.erase(i);
		}
	}

	getInstance()._registry.emplace(entity->name(), entity);

	entity->activate();

	return std::move(entity);
}

std::shared_ptr<Service> Services::get(const std::string& name)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._registry.find(name);
	if (i == getInstance()._registry.end())
	{
		return std::shared_ptr<Service>();
	}

	return i->second;
}

void Services::del(const std::string& name)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._registry.find(name);
	if (i == getInstance()._registry.end())
	{
		return;
	}

	i->second->deactivate();

	getInstance()._registry.erase(i);
}

void Services::activateAll()
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);

	for (const auto& i : getInstance()._registry)
	{
		i.second->activate();
	}
}

void Services::deactivateAll()
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);

	for (const auto& i : getInstance()._registry)
	{
		i.second->deactivate();
	}
}
