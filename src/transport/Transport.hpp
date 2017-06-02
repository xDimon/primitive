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
// File created on: 2017.02.26

// Transport.hpp


#pragma once

#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"

#include "../log/Log.hpp"
#include "../net/AcceptorFactory.hpp"
#include "../configs/Setting.hpp"
#include "../serialization/SerializerFactory.hpp"
#include "../utils/Context.hpp"

#include <memory>
#include <functional>

class Transport : public Shareable<Transport>, public Named
{
private:
	std::shared_ptr<AcceptorFactory::Creator> _acceptorCreator;
	std::weak_ptr<Connection> _acceptor;

protected:
	Log _log;

public:

	Transport(const Setting& setting);
	virtual ~Transport();

	bool enable();
	bool disable();

	virtual bool processing(std::shared_ptr<Connection> connection) = 0;

	typedef std::function<void(const char*, size_t, bool)> Transmitter;
	typedef std::function<void(std::shared_ptr<Context>, const char*, size_t, Transmitter)> Handler;

	virtual void bindHandler(const std::string& selector, Handler handler) = 0;
	virtual Handler getHandler(std::string subject) = 0;
};

#include "TransportFactory.hpp"

#define REGISTER_TRANSPORT(Type,Class) const bool Class::__dummy = \
    TransportFactory::reg(                                                                      \
        #Type,                                                                                  \
        [](const Setting& setting){                                                             \
            return std::shared_ptr<Transport>(new Class(setting));                              \
        }                                                                                       \
    );

#define DECLARE_TRANSPORT(Class) \
private:                                                                                        \
    Class() = delete;                                                                           \
    Class(const Class&) = delete;                                                               \
    void operator=(Class const&) = delete;                                                      \
                                                                                                \
    Class(const Setting& setting)                                                               \
    : Transport(setting)                                                                        \
    {                                                                                           \
        _log.setName(#Class);                                                                   \
        _log.debug("Transport '%s' created", name().c_str());                                   \
    }                                                                                           \
                                                                                                \
public:                                                                                         \
    virtual ~Class()                                                                            \
    {                                                                                           \
        _log.debug("Transport '%s' destroyed", name().c_str());                                 \
    }                                                                                           \
                                                                                                \
    virtual bool processing(std::shared_ptr<Connection> connection) override;                   \
    virtual void bindHandler(const std::string& selector, Transport::Handler Handler) override; \
    virtual Transport::Handler getHandler(std::string subject) override;                        \
                                                                                                \
private:                                                                                        \
    static const bool __dummy;
