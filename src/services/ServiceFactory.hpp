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
// File created on: 2017.05.31

// ServiceFactory.hpp


#pragma once


#include <map>
#include "Service.hpp"
#include "../utils/Dummy.hpp"

class ServiceFactory final
{
public:
	ServiceFactory(const ServiceFactory&) = delete;
	ServiceFactory& operator=(ServiceFactory const&) = delete;
	ServiceFactory(ServiceFactory&&) noexcept = delete;
	ServiceFactory& operator=(ServiceFactory&&) noexcept = delete;

private:
	ServiceFactory() = default;
	~ServiceFactory() = default;

	static ServiceFactory& getInstance()
	{
		static ServiceFactory instance;
		return instance;
	}

	std::map<std::string, std::shared_ptr<Service>(*)(const Setting &setting)> _creators;

public:
	static Dummy reg(
		const std::string& type,
		std::shared_ptr<Service>(*)(const Setting &setting)
	) noexcept;
	static std::shared_ptr<Service> create(const Setting& setting);
};
