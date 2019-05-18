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
// File created on: 2017.02.26

// ServerTransport.hpp


#pragma once

#include "Transport.hpp"
#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"

#include "../log/Log.hpp"
#include "../net/AcceptorFactory.hpp"
#include "../configs/Setting.hpp"
#include "../serialization/SerializerFactory.hpp"
#include "../utils/Context.hpp"
#include "../telemetry/Metric.hpp"

#include <memory>
#include <functional>

class ServerTransport : public Shareable<ServerTransport>, public Transport
{
private:
	std::shared_ptr<AcceptorFactory::Creator> _acceptorCreator;
	std::weak_ptr<Connection> _acceptor;

public:
	ServerTransport() = delete;
	ServerTransport(const ServerTransport&) = delete;
    ServerTransport& operator=(const ServerTransport&) = delete;
	ServerTransport(ServerTransport&&) noexcept = delete;
	ServerTransport& operator=(ServerTransport&&) noexcept = delete;

	explicit ServerTransport(const Setting& setting);
	~ServerTransport() override = default;

	std::shared_ptr<Metric> metricConnectCount;
	std::shared_ptr<Metric> metricRequestCount;
	std::shared_ptr<Metric> metricAvgRequestPerSec;
	std::shared_ptr<Metric> metricAvgExecutionTime;

	virtual bool enable() final;
	virtual bool disable() final;

	virtual void bindHandler(const std::string& selector, const std::shared_ptr<Handler>& handler) = 0;
	virtual void unbindHandler(const std::string& selector) = 0;
	virtual std::shared_ptr<Handler> getHandler(const std::string& subject) = 0;
};

#include "TransportFactory.hpp"

#define REGISTER_TRANSPORT(Type,Class) const Dummy Class::__dummy = \
    TransportFactory::reg(                                                                      \
        #Type,                                                                                  \
        [](const Setting& setting){                                                             \
            return std::shared_ptr<ServerTransport>(new Class(setting));                        \
        }                                                                                       \
    );

#define DECLARE_TRANSPORT(Class) \
public:                                                                                         \
	Class() = delete;                                                                           \
	Class(const Class&) = delete;                                                               \
    Class& operator=(const Class&) = delete;                                                    \
	Class(Class&&) noexcept = delete;                                                           \
	Class& operator=(Class&&) noexcept = delete;                                                \
                                                                                                \
private:                                                                                        \
    explicit Class(const Setting& setting)                                                      \
    : ServerTransport(setting)                                                                  \
    {                                                                                           \
        _log.setName(#Class);                                                                   \
        _log.debug("Transport '%s' created", name().c_str());                                   \
    }                                                                                           \
                                                                                                \
public:                                                                                         \
    ~Class() override                                                                           \
    {                                                                                           \
        _log.debug("Transport '%s' destroyed", name().c_str());                                 \
    }                                                                                           \
                                                                                                \
    bool processing(const std::shared_ptr<Connection>& connection) override;                    \
    void bindHandler(const std::string& selector, const std::shared_ptr<Handler>&) override;    \
    void unbindHandler(const std::string& selector) override;                                   \
    std::shared_ptr<Handler> getHandler(const std::string& subject) override;                   \
                                                                                                \
private:                                                                                        \
    static const Dummy __dummy;
