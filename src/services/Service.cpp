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
// File created on: 2017.05.30

// Service.cpp


#include <sstream>
#include "Service.hpp"

Service::Service(const Setting& setting)
: _log("Service")
, _setting(setting)
{
	if (setting.exists("name"))
	{
		setting.lookupValue("name", _name);
	}
	if (_name.empty() && setting.exists("type"))
	{
		std::string type;
		setting.lookupValue("type", type);

		std::ostringstream ss;
		ss << "_service[" << type << "]";
		_name = std::move(ss.str());
	}
	if (_name.empty())
	{
		std::ostringstream ss;
		ss << "_service[" << std::hex << this << "]";
		_name = std::move(ss.str());
	}
}
