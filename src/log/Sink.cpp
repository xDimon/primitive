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
// File created on: 2017.08.16

// Sink.cpp


#include <stdexcept>
#include "Sink.hpp"
#include <sstream>

Sink::Sink(const Setting& setting)
{
	std::string cfgline = "/P7.Pool=32768";

	if (!setting.lookupValue("name", _name))
	{
		throw std::runtime_error("Not found name for one of sinks");
	}

	std::string type;
	if (!setting.lookupValue("type", type))
	{
		throw std::runtime_error("Undefined type for sink '" + _name + "'");
	}
	if (type == "console")
	{
		cfgline += " /P7.Sink=Console";
	}
	else if (type == "file")
	{
		cfgline += " /P7.Sink=FileTxt";
	}
	else if (type == "syslog")
	{
		cfgline += " /P7.Sink=Syslog";
	}
	else
	{
		throw std::runtime_error("Unknown type ('" + type + "') for sink '" + _name + "'");
	}

	std::string format;
	if (!setting.lookupValue("format", format))
	{
		format = "%tm\t%tn\t%mn\t%lv\t%ms";
	}
	cfgline += " /P7.Format=" + format;

	if (type == "file")
	{
		std::string directory;
		if (!setting.lookupValue("directory", directory))
		{
			throw std::runtime_error("Undefined directory for sink '" + _name + "'");
		}
		cfgline += " /P7.Dir=" + directory;

		std::string rolling;
		if (!setting.lookupValue("rolling", rolling))
		{
			throw std::runtime_error("Undefined setting of rolling for sink '" + _name + "'");
		}
		cfgline += " /P7.Roll=" + rolling;

		uint32_t maxcount;
		if (!setting.lookupValue("maxcount", maxcount))
		{
			throw std::runtime_error("Undefined maxcount of rolling for sink '" + _name + "'");
		}
		cfgline += " /P7.Files=" + maxcount;
	}

	_p7client = P7_Create_Client(cfgline.c_str());
	if (_p7client == nullptr)
	{
		throw std::runtime_error("Can't create p7-client for sink '" + _name + "'");
	}

	_p7trace = P7_Create_Trace(_p7client, "TraceChannel");
	if (_p7trace == nullptr)
	{
		throw std::runtime_error("Can't create p7-channel for sink '" + _name + "'");
	}
	_p7trace->Share("TraceChannel");
}

Sink::Sink()
{
	_p7client = P7_Create_Client("/P7.Pool=32768 /P7.Sink=Console /P7.Format=%tm\t%tn\t%mn\t%lv\t%ms");
	if (_p7client == nullptr)
	{
		throw std::runtime_error("Can't create p7-client for default sink");
	}

	_p7trace = P7_Create_Trace(_p7client, "TraceChannel");
	if (_p7trace == nullptr)
	{
		throw std::runtime_error("Can't create p7-channel for default sink");
	}
	_p7trace->Share("TraceChannel");
}

Sink::~Sink()
{
	_p7trace->Release();
	_p7client->Release();
}
