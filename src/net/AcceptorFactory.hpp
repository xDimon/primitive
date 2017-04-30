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
// File created on: 2017.04.30

// AcceptorFactory.hpp


#pragma once

#include <functional>
#include <memory>

#include "../configs/Setting.hpp"

class Connection;
class Acceptor;
class Transport;

class AcceptorFactory
{
public:
	typedef std::function<std::shared_ptr<Connection>(std::shared_ptr<Transport>&)> Creator;

private:
	AcceptorFactory()
	{};

	virtual ~AcceptorFactory()
	{};

	AcceptorFactory(AcceptorFactory const&) = delete;

	void operator=(AcceptorFactory const&) = delete;

	static AcceptorFactory& getInstance()
	{
		static AcceptorFactory instance;
		return instance;
	}

public:
	std::shared_ptr<Acceptor> create(const Setting& setting);

	static std::shared_ptr<AcceptorFactory::Creator> creator(const Setting& setting);
};
