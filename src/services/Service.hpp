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
// File created on: 2017.05.30

// Service.hpp


#pragma once


#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"
#include "../serialization/SerializerFactory.hpp"
#include "../log/Log.hpp"

class Server;

class Service : public Shareable<Service>, public Named
{
protected:
	Log _log;
	const Setting& _setting;

	Service(const Setting& setting);

public:
	virtual ~Service();

	virtual void activate(Server *server) = 0;
	virtual void deactivate(Server *server) = 0;
};

#include "ServiceFactory.hpp"

#define REGISTER_SERVICE(Type,Class) const bool Class::__dummy = \
	ServiceFactory::reg(												\
		#Type, 															\
		[](const Setting& setting){										\
			return std::shared_ptr<Service>(new Class(setting));		\
		}																\
	);

#define DECLARE_SERVICE(Class) \
private:																\
	Class() = delete;													\
	Class(const Class&) = delete;										\
	void operator=(Class const&) = delete;								\
																		\
	Class(const Setting& setting);										\
																		\
public:																	\
	virtual ~Class();													\
	virtual void activate(Server *server);								\
	virtual void deactivate(Server *server);							\
																		\
private:																\
	static const bool __dummy;
