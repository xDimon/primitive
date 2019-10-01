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
	mutable Log _log;

	std::vector<std::shared_ptr<ServicePart>> _parts;

	explicit Service(const Setting& setting);

public:
	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;
	Service(Service&&) noexcept = delete;
	Service& operator=(Service&&) noexcept = delete;

	~Service() override;

	Log& log() const
	{
		return _log;
	}

	virtual const std::string& type() const = 0;

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
	);                                                                      \
const std::string Type::Service::_type(#Type);                              \

#define DECLARE_SERVICE() \
private:                                                                    \
	static const std::string _type;                                         \
                                                                            \
public:                                                                     \
	Service() = delete;                                                     \
	Service(const Service&) = delete;                                       \
    Service& operator=(const Service&) = delete;                            \
	Service(Service&&) noexcept = delete;                                   \
	Service& operator=(Service&&) noexcept = delete;                        \
                                                                            \
private:                                                                    \
    explicit Service(const Setting& setting);                               \
                                                                            \
public:                                                                     \
    ~Service() override = default;                                          \
                                                                            \
	const std::string& type() const override                                \
	{                                                                       \
		return _type;                                                       \
	}                                                                       \
    void activate() override;                                               \
    void deactivate() override;                                             \
                                                                            \
private:                                                                    \
    static const Dummy __dummy;
