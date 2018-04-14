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
// File created on: 2017.05.31

// ActionFactory.hpp


#pragma once


#include <string>
#include <map>
#include <memory>
#include "Action.hpp"
#include "../transport/ServerTransport.hpp"
#include "../utils/Dummy.hpp"

class ActionFactory final
{
public:
	ActionFactory(const ActionFactory&) = delete;
	ActionFactory& operator=(ActionFactory const&) = delete;
	ActionFactory(ActionFactory&&) noexcept = delete;
	ActionFactory& operator=(ActionFactory&&) noexcept = delete;

private:
	ActionFactory() = default;
	~ActionFactory() = default;

	static ActionFactory& getInstance()
	{
		static ActionFactory instance;
		return instance;
	}

	std::map<
		std::string,
		std::shared_ptr<Action>(*)(
			const std::shared_ptr<ServicePart>&,
			const std::shared_ptr<Context>&,
			const SVal&
		)
	> _creators;

public:
	static Dummy reg(
		const std::string& name,
		std::shared_ptr<Action>(* creator)(
			const std::shared_ptr<ServicePart>&,
			const std::shared_ptr<Context>&,
			const SVal&
		)
	) noexcept;
	static std::shared_ptr<Action> create(
		const std::string& name,
		const std::shared_ptr<ServicePart>& servicePart,
		const std::shared_ptr<Context>& context,
		const SVal& input
	);
};
