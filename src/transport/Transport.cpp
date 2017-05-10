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
// File created on: 2017.02.26

// Transport.cpp


#include <sstream>
#include "Transport.hpp"
#include "../net/ConnectionManager.hpp"

Transport::Transport(const Setting& setting)
{
	std::string name;
	if (setting.exists("name"))
	{
		setting.lookupValue("name", name);
	}

	if (!name.length())
	{
		std::ostringstream ss;
		ss << "_transport#" << this;
		name = std::move(ss.str());
	}

	_name = std::move(name);

	_acceptorCreator = AcceptorFactory::creator(setting);
	_serializerCreator = SerializerFactory::creator(setting);
}

bool Transport::enable()
{
	if (!_acceptor.expired())
	{
		log().trace_("Transport '%s' already enabled", name().c_str());
		return true;
	}

	try
	{
		auto t = this->ptr();
		auto acceptor = (*_acceptorCreator)(t);

		_acceptor = acceptor->ptr();

		ConnectionManager::add(_acceptor.lock());

		log().debug("Transport '%s' enabled", name().c_str());

		return true;
	}
	catch (std::runtime_error exception)
	{
		log().debug("Transport '%s' didn't enable: Can't create Acceptor: %s", name().c_str(), exception.what());

		return false;
	}
}

bool Transport::disable()
{
	if (_acceptor.expired())
	{
		log().debug("Transport '%s' not enable", name().c_str());
		return true;
	}

	ConnectionManager::remove(_acceptor.lock());

	_acceptor.reset();

	log().debug("Transport '%s' disabled", name().c_str());
	return true;
}
