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
// File created on: 2017.09.13

// Services.hpp


#pragma once


#include <mutex>
#include "Service.hpp"

class Services final
{
public:
	Services(const Services&) = delete; // Copy-constructor
	Services& operator=(Services const&) = delete; // Copy-assignment
	Services(Services&&) noexcept = delete; // Move-constructor
	Services& operator=(Services&&) noexcept = delete; // Move-assignment

private:
	Services() = default;
	~Services() = default;

	static Services &getInstance()
	{
		static Services instance;
		return instance;
	}

	std::map<std::string, const std::shared_ptr<Service>> _registry;
	std::recursive_mutex _mutex;

public:
	static std::shared_ptr<Service> add(const Setting& setting, bool replace = false);
	static std::shared_ptr<Service> get(const std::string& name);
	static void del(const std::string& name);

	static void activateAll();
	static void deactivateAll();
};
