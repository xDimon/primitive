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


#include <sstream>
#include "Service.hpp"

static uint32_t id4noname = 0;

Service::Service(const Setting& setting)
: _log("service[" + std::to_string(++id4noname) + "_unknown]")
{
	if (setting.has("name"))
	{
		_name = setting.getAs<SStr>("name");
	}
	if (_name.empty())
	{
		if (setting.has("type"))
		{
			std::string type = setting.getAs<SStr>("type");

			_name = "service[" + std::to_string(id4noname) + "_" + type + "]";
		}
		else
		{
			_name = "service[" + std::to_string(id4noname) + "_unknown]";
		}
	}

	_log.setName(_name);
	_log.debug("Service '%s' created", _name.c_str());
}

Service::~Service()
{
	_log.debug("Service '%s' destroyed", _name.c_str());
}
