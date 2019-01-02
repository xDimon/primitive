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

// Applications.cpp


#include "Applications.hpp"

std::shared_ptr<Application> Applications::add(const Setting& setting, bool replace)
{
	std::string appId;
	if (!setting.lookupValue("appId", appId) || appId.empty())
	{
		throw std::runtime_error("Field appId undefined");
	}

	if (!replace)
	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		if (getInstance()._apps.find(appId) != getInstance()._apps.end())
		{
			throw std::runtime_error(std::string("Already exists application with the same appId ('") + appId + "')");
		}
	}

	auto application = ApplicationFactory::create(setting);

	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
	if (!replace)
	{
		auto i = getInstance()._apps.find(application->appId());
		if (i != getInstance()._apps.end())
		{
			getInstance()._apps.erase(i);
		}
	}

	getInstance()._apps.emplace(application->appId(), application);

	return std::move(application);
}

std::shared_ptr<Application> Applications::get(const std::string& appId)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._apps.find(appId);
	if (i == getInstance()._apps.end())
	{
		return std::shared_ptr<Application>();
	}

	return i->second;
}

void Applications::del(const std::string& appId)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	auto i = getInstance()._apps.find(appId);
	if (i == getInstance()._apps.end())
	{
		return;
	}

	getInstance()._apps.erase(i);
}
