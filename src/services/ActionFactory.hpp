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
// File created on: 2017.05.31

// ActionFactory.hpp


#pragma once


#include <string>
#include <map>
#include <memory>
#include "Action.hpp"
#include "../transport/ServerTransport.hpp"

class ActionFactory
{
private:
	ActionFactory()
	{};

	virtual ~ActionFactory()
	{};

	ActionFactory(ActionFactory const&) = delete;

	void operator=(ActionFactory const&) = delete;

	static ActionFactory& getInstance()
	{
		static ActionFactory instance;
		return instance;
	}

	std::map<
		std::string,
		std::shared_ptr<Action>(*)(
			const std::shared_ptr<Service>&,
			const std::shared_ptr<Context>&,
			const SVal*,
			ServerTransport::Transmitter
		)
	> _creators;

public:
	static bool reg(
		const std::string& name,
		std::shared_ptr<Action>(* creator)(
			const std::shared_ptr<Service>&,
			const std::shared_ptr<Context>&,
			const SVal*,
			ServerTransport::Transmitter
		)
	);
	static std::shared_ptr<Action> create(
		const std::string& name,
		const std::shared_ptr<Service>& service,
		const std::shared_ptr<Context>& context,
		const SVal* input,
		ServerTransport::Transmitter transmitter
	);
};
