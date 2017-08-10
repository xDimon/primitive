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
// File created on: 2017.07.02

// Transports.hpp


#pragma once


#include "ServerTransport.hpp"
#include "../configs/Setting.hpp"

#include <map>
#include <mutex>

class Transports final
{
public:
	Transports(Transports const&) = delete;
	void operator=(Transports const&) = delete;
	Transports(Transports&&) = delete;
	Transports& operator=(Transports&&) = delete;

private:
	Transports() = default;
	~Transports() = default;

	static Transports &getInstance()
	{
		static Transports instance;
		return instance;
	}

	std::map<std::string, const std::shared_ptr<ServerTransport>> _registry;
	std::mutex _mutex;

public:
	static std::shared_ptr<ServerTransport> add(const Setting& setting, bool replace = false);
	static std::shared_ptr<ServerTransport> get(const std::string& name);
	static void del(const std::string& name);

	static void enable(const std::string& name);
	static void disable(const std::string& name);

	static void enableAll();
	static void disableAll();
};
