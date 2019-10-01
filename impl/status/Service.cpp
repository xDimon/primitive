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
// File created on: 2017.05.30

// Service.cpp


#include "Service.hpp"
#include "ClientPart.hpp"
#include <serialization/SArr.hpp>

REGISTER_SERVICE(status)

status::Service::Service(const Setting& setting)
: ::Service(setting)
{
	SArr partsConfig;

	try
	{
		if (!setting.has("parts"))
		{
			throw std::runtime_error("Not found parts' configs");
		}

		for (const auto& config_ : setting.getAs<SArr>("parts"))
		{
			auto& config = config_.as<SObj>();

			const std::string& type = config.getAs<SStr>("type");
			if (type.empty())
			{
				throw std::runtime_error("Field type is empty");
			}

			std::shared_ptr<ServicePart> part;

			if (type == "client")
			{
				part = std::make_shared<ClientPart>(ptr());
			}
			else
			{
				throw std::runtime_error("Unknown part with type '" + type + "'");
			}

			part->init(config);

			partsConfig.push_back(config);
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error("Can't set up service '" + name() + "' ← " + exception.what());
	}

	_partsConfig = std::move(partsConfig);
}

void status::Service::activate()
{
	try
	{
		for (const auto& config_ : _partsConfig)
		{
			auto& config = config_.as<SObj>();

			const std::string& type = config.getAs<SStr>("type");

			std::shared_ptr<ServicePart> part;

			if (type == "client")
			{
				part = std::make_shared<ClientPart>(ptr());
			}

			part->init(config);
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error("Can't activate service '" + name() + "' ← " + exception.what());
	}
}

void status::Service::deactivate()
{
	for (auto& part : _parts)
	{
		part->deinit();
	}
	_parts.clear();
}
