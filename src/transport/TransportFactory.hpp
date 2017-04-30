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
// File created on: 2017.04.29

// TransportFactory.hpp


#pragma once

#include <memory>
#include "../net/Connection.hpp"
#include "../configs/Setting.hpp"

class TransportFactory
{
private:
	TransportFactory()
	{};

	virtual ~TransportFactory()
	{};

	TransportFactory(TransportFactory const&) = delete;

	void operator=(TransportFactory const&) = delete;

	static TransportFactory& getInstance()
	{
		static TransportFactory instance;
		return instance;
	}

	std::shared_ptr<Transport> createHttp(const Setting& setting);

	std::shared_ptr<Transport> createWs(const Setting& setting);

	std::shared_ptr<Transport> createPacket(const Setting& setting);

public:
	static std::shared_ptr<Transport> create(const Setting& setting);
};
