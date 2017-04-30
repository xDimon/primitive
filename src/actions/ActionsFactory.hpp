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

// ActionsFactory.hpp


#pragma once

#include <map>

#include "RequestBase.hpp"

class ActionsFactory final
{
private:
	ActionsFactory() {};
	~ActionsFactory() {};
	ActionsFactory(ActionsFactory const&) = delete;
	void operator= (ActionsFactory const&) = delete;

	static ActionsFactory &getInstance()
	{
		static ActionsFactory instance;
		return instance;
	}

	std::map<const std::string, RequestBase *(*)(std::shared_ptr<Connection>&, const void *)> _requests;

	std::string _regRequest(const std::string& name, RequestBase *(*creator)(std::shared_ptr<Connection>&, const void *));
	RequestBase *_createRequest(const std::string& name, std::shared_ptr<Connection>& connection, const void *request);

public:
	static const std::string regRequest(const std::string& name, RequestBase *(*creator)(std::shared_ptr<Connection>&, const void *))
	{
		return getInstance()._regRequest(name, creator);
	}
	static RequestBase *createRequest(const std::string& name, std::shared_ptr<Connection>& connection, const void *data)
	{
		return getInstance()._createRequest(name, connection, data);
	}
};
