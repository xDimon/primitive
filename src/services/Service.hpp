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
#include <set>
#include "ServicePart.hpp"
#include "../serialization/SerializerFactory.hpp"
#include "../log/Log.hpp"

class Server;

class Service : public Shareable<Service>, public Named
{
protected:
	Log _log;
	const Setting& _setting;

	std::set<std::shared_ptr<ServicePart>, ServicePart::Comparator> _parts;

	explicit Service(const Setting& setting);

public:
	Service(const Service&) = delete;
	void operator=(Service const&) = delete;
	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

	virtual ~Service();

	virtual void activate() = 0;
	virtual void deactivate() = 0;
};

#include "ServiceFactory.hpp"

#define REGISTER_SERVICE(Type) const Dummy Type::Service::__dummy = \
	ServiceFactory::reg(                                                    \
		#Type,                                                              \
		[](const Setting& setting){                                         \
			return std::shared_ptr<::Service>(new Type::Service(setting));  \
		}                                                                   \
	);

#define DECLARE_SERVICE() \
public:                                                                     \
	Service() = delete;                                                     \
	Service(const Service&) = delete;                                       \
    void operator=(Service const&) = delete;                                \
	Service(Service&&) = delete;                                            \
	Service& operator=(Service&&) = delete;                                 \
                                                                            \
private:                                                                    \
    explicit Service(const Setting& setting);                               \
                                                                            \
public:                                                                     \
    ~Service() override = default;                                          \
                                                                            \
    void activate() override;                                               \
    void deactivate() override;                                             \
                                                                            \
private:                                                                    \
    static const Dummy __dummy;
