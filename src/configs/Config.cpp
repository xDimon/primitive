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

#include <libconfig.h++>
#include <sstream>

Config::Config(Options::Ptr &options)
: _options(options)
//, _mainConfig(new libconfig::Config())
//, _transportConfig(new libconfig::Config())
{
	auto &configPath = _options->getConfig();

	try
	{
		auto&& root = getRoot();
		{
			auto &&transports = root.add("transports", libconfig::Setting::Type::TypeList);
			{
				auto &&transport = transports.add(libconfig::Setting::Type::TypeGroup);

				transport.add("name", libconfig::Setting::Type::TypeString) = "http::8000";
				transport.add("type", libconfig::Setting::Type::TypeString) = "http";
				transport.add("secure", libconfig::Setting::Type::TypeBoolean) = false;
				transport.add("address", libconfig::Setting::Type::TypeString) = "0.0.0.0";
				transport.add("port", libconfig::Setting::Type::TypeInt) = 8000;
				transport.add("serialization", libconfig::Setting::Type::TypeString) = "json";
			}
			{
				auto &&transport = transports.add(libconfig::Setting::Type::TypeGroup);

				transport.add("name", libconfig::Setting::Type::TypeString) = "https::4430";
				transport.add("type", libconfig::Setting::Type::TypeString) = "http";
				transport.add("secure", libconfig::Setting::Type::TypeBoolean) = true;
				transport.add("address", libconfig::Setting::Type::TypeString) = "0.0.0.0";
				transport.add("port", libconfig::Setting::Type::TypeInt) = 4430;
				transport.add("serialization", libconfig::Setting::Type::TypeString) = "json";
			}
			{
				auto &&transport = transports.add(libconfig::Setting::Type::TypeGroup);

				transport.add("name", libconfig::Setting::Type::TypeString) = "wss::4431";
				transport.add("type", libconfig::Setting::Type::TypeString) = "websocket";
				transport.add("secure", libconfig::Setting::Type::TypeBoolean) = true;
				transport.add("address", libconfig::Setting::Type::TypeString) = "0.0.0.0";
				transport.add("port", libconfig::Setting::Type::TypeInt) = 4431;
				transport.add("serialization", libconfig::Setting::Type::TypeString) = "json";
			}
			{
				auto &&transport = transports.add(libconfig::Setting::Type::TypeGroup);

				transport.add("type", libconfig::Setting::Type::TypeString) = "packet";
				transport.add("secure", libconfig::Setting::Type::TypeBoolean) = true;
				transport.add("address", libconfig::Setting::Type::TypeString) = "0.0.0.0";
				transport.add("port", libconfig::Setting::Type::TypeInt) = 8002;
				transport.add("serialization", libconfig::Setting::Type::TypeString) = "json";
			}
		}

		libconfig::Config::writeFile(configPath.c_str());
	}
	catch (libconfig::SettingTypeException& exception)
	{
		throw;
	}
	catch (...)
	{
		throw;
	}



//
//		type = http;
//		secure = false;
//		address = 0.0.0.0;
//		port = 8000;
//		serialization = json;




















	try
	{
//		if (!libconfig::Config::exists(configPath))
//		{
//			throw std::runtime_error("Config file not found");
//		}
		libconfig::Config::readFile(configPath.c_str());
	}
	catch (const libconfig::FileIOException &fioex)
	{
		std::ostringstream ss;
		ss << "I/O error while reading config file: " << fioex.what();
		throw std::runtime_error(ss.str());
	}
	catch (const libconfig::ParseException &pex)
	{
		std::ostringstream ss;
		ss << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError();
		throw std::runtime_error(ss.str());
	}
}

Config::~Config()
{

}
