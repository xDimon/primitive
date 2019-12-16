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
#include <climits>
#include <unistd.h>
#include <libgen.h>

void Config::init(const std::shared_ptr<Options>& options)
{
	if (!options)
	{
		throw std::runtime_error("Bad options");
	}

	auto& config = getInstance();

	config._options = options;

	try
	{
		auto& configFile = config._options->configFile();
		if (configFile.empty())
		{
			throw std::runtime_error("Filename of config is empty");
		}

		config._settings = read(configFile);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't get config ← " + exception.what());
	}
}

SObj Config::read(std::string path)
{
	SObj config;

	if (path.empty())
	{
		return config;
	}

	if (path[0] != '/') // relative path
	{
		char buf[PATH_MAX];
		std::string cwd = getcwd(buf, sizeof(buf));
		path = cwd + '/' + path;
	}

	std::ifstream ifs(path);

	if (!ifs.is_open())
	{
		throw std::runtime_error("Can't open file '" + path + "' ← " + strerror(errno));
	}

	try
	{
		config = SerializerFactory::create("json")->decode(ifs).as<SObj>();

		if (config.has("include"))
		{
			char buf[PATH_MAX];
			std::string cwd = getcwd(buf, sizeof(buf));

			strncpy(buf, path.c_str(), sizeof(buf));
			chdir(dirname(buf));

			processIncludes(config);

			chdir(cwd.c_str());
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't process file '" + path + "' ← " + exception.what());
	}

	return config;
}

void Config::processIncludes(SObj& config)
{
	std::vector<std::string> paths;
	try
	{
		auto includes = config.extractAs<SArr>("include");

		for (auto& include : includes)
		{
			paths.emplace_back(include.as<SStr>().value());
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't process includes ← " + exception.what());
	}

	for (auto& path : paths)
	{
		try
		{
			auto includeConfig = read(path);

			for (auto& item : includeConfig)
			{
				config[item.first] = std::move(item.second);
			}
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error(std::string() + "Can't process include '" + path + "' ← " + exception.what());
		}
	}
}
