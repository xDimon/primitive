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
// File created on: 2017.07.02

// Application.hpp


#pragma once


#include "../utils/Shareable.hpp"
#include "../configs/Setting.hpp"
#include <string>

class Application : public Shareable<Application>
{
public:
	typedef std::string Type;
	typedef std::string Id;

protected:
	Type _type;
	Id _appId;
	bool _isProduction;

public:
	Application() = delete;
	Application(const Application&) = delete;
    Application& operator=(Application const&) = delete;
	Application(Application&&) noexcept = delete;
	Application& operator=(Application&&) noexcept = delete;

	explicit Application(const Setting& setting);
	~Application() override = default;

	const Type& type() const
	{
		return _type;
	}

	const Id& appId() const
	{
		return _appId;
	}

	bool isProduction() const
	{
		return _isProduction;
	}
};

#include "ApplicationFactory.hpp"

#define REGISTER_APPLICATION(Type,Class) const Dummy Class::__dummy = \
    ApplicationFactory::reg(                                                                    \
        #Type,                                                                                  \
        [](const Setting& setting){                                                             \
            return std::shared_ptr<::Application>(new Class(setting));                          \
        }                                                                                       \
    );

#define DECLARE_APPLICATION(Class) \
public:                                                                                         \
	Class() = delete;                                                                           \
	Class(const Class&) = delete;                                                               \
    Class& operator=(Class const&) = delete;                                                    \
	Class(Class&&) noexcept = delete;                                                           \
	Class& operator=(Class&&) noexcept = delete;                                                \
    ~Class() override = default;                                                                \
                                                                                                \
private:                                                                                        \
    Class(const Setting& setting);                                                              \
                                                                                                \
private:                                                                                        \
    static const Dummy __dummy;
