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
// File created on: 2017.02.25

// Config.cpp


#include "Config.hpp"

#include <fstream>
#include <serialization/JsonSerializer.hpp>

Config::Config(const std::shared_ptr<Options>& options)
: _options(options)
{
	if (!_options)
	{
		throw std::runtime_error("Bad options");
	}

	try
	{
		auto& configFile = _options->configFile();
		if (configFile.empty())
		{
			throw std::runtime_error("Filename of config is empty");
		}

		std::ifstream ifs(configFile);

		if (!ifs.is_open())
		{
			throw std::runtime_error("Can't open config file '" + configFile + "' ← " + strerror(errno));
		}

		_settings = SerializerFactory::create("json")->decode(ifs).as<SObj>();
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't get config ← " + exception.what());
	}
}
