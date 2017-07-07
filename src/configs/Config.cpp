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
// File created on: 2017.02.25

// Config.cpp


#include "Config.hpp"

#include <sstream>

Config::Config(const std::shared_ptr<Options>& options)
: _options(options)
{
	if (!_options)
	{
		throw std::runtime_error("Bad options");
	}

	auto& configPath = _options->getConfig();

	try
	{
		libconfig::Config::readFile(configPath.c_str());
	}
	catch (const libconfig::ParseException& exception)
	{
		std::ostringstream ss;
		ss << "Parse error at " << exception.getFile() << ":" << exception.getLine() << " - " << exception.getError();
		throw std::runtime_error(ss.str());
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't read config file: " + exception.what());
	}
}

Config::~Config()
{
}
