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
// File created on: 2017.07.02

// Transports.cpp


#include "Transports.hpp"

std::shared_ptr<ServerTransport> Transports::add(const Setting& setting, bool replace)
{
	std::string name;
	if (!setting.lookupValue("name", name) || name.empty())
	{
		throw std::runtime_error("Field name undefined");
	}

	if (!replace)
	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		if (getInstance()._registry.find(name) != getInstance()._registry.end())
		{
			throw std::runtime_error(std::string("Already exists transport with the same name ('") + name + "')");
		}
	}

	auto entity = TransportFactory::create(setting);

	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
	if (!replace)
	{
		auto i = getInstance()._registry.find(entity->name());
		if (i != getInstance()._registry.end())
		{
			getInstance()._registry.erase(i);
		}
	}

	getInstance()._registry.emplace(entity->name(), entity);

	return std::move(entity);
}

std::shared_ptr<ServerTransport> Transports::get(const std::string& name)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._registry.find(name);
	if (i == getInstance()._registry.end())
	{
		return std::shared_ptr<ServerTransport>();
	}

	return i->second;
}

void Transports::del(const std::string& name)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._registry.find(name);
	if (i == getInstance()._registry.end())
	{
		return;
	}

	i->second->disable();

	getInstance()._registry.erase(i);
}

void Transports::enable(const std::string& name)
{
	auto transport = get(name);
	if (!transport)
	{
		return;
	}
	transport->enable();
}

void Transports::disable(const std::string& name)
{
	auto transport = get(name);
	if (!transport)
	{
		return;
	}
	transport->disable();
}

void Transports::enableAll()
{
	for (auto i : getInstance()._registry)
	{
		i.second->enable();
	}
}

void Transports::disableAll()
{
	for (auto i : getInstance()._registry)
	{
		i.second->disable();
	}
}
