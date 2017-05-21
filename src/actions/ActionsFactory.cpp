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
// File created on: 2017.04.06

// ActionsFactory.cpp


#include "ActionsFactory.hpp"

#include <assert.h>

std::string ActionsFactory::_regRequest(const std::string& name, RequestBase *(*creator)(std::shared_ptr<Connection>&, const void *))
{
	auto i = _requests.find(name);
	assert(i == _requests.end());
	if (i != _requests.end())
	{
		throw std::runtime_error("Double registered");
	}
	_requests.emplace(std::make_pair(name, creator));
	return name;
}

RequestBase *ActionsFactory::_createRequest(const std::string& name, std::shared_ptr<Connection>& connection, const void *data)
{
	auto i = _requests.find(name);
	if (i == _requests.end())
	{
		return nullptr;
	}
	return i->second(connection, data);
}
