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
// File created on: 2017.05.31

// Action.hpp


#pragma once


#include <cstdint>
#include "../utils/Context.hpp"
#include "../serialization/SBase.hpp"
#include "../serialization/SObj.hpp"
#include "../transport/ServerTransport.hpp"
#include "Service.hpp"

class Action
{
protected:
	using Id = int64_t;

	std::string _actionName;

	std::shared_ptr<Service> _service;
	std::shared_ptr<ServicePart> _servicePart;
	std::shared_ptr<Context> _context;
	SVal _data;
	Id _requestId;
	Id _lastConfirmedEvent;
	Id _lastConfirmedResponse;
	mutable bool _answerSent;

	virtual bool validate() = 0;
	virtual bool execute() = 0;

public:
	Action() = delete;
	Action(const Action&) = delete;
	Action& operator=(Action const&) = delete;
	Action(Action&&) noexcept = delete;
	Action& operator=(Action&&) noexcept = delete;

	Action(
		const std::shared_ptr<ServicePart>& servicePart,
		const std::shared_ptr<Context>& context,
		const SVal& input
	);
	virtual ~Action() = default;

	inline auto name()
	{
		return _actionName;
	}

	bool doIt(const std::string& where, std::chrono::steady_clock::time_point beginExecTime = std::chrono::steady_clock::now());

	std::shared_ptr<ServicePart> servicePart() const
	{
		return _servicePart;
	}

	std::shared_ptr<Service> service() const
	{
		return _service;
	}

	virtual SObj response(SVal&& data) const;
	inline SObj response() const
	{
		return response(SVal());
	}

	virtual SObj error(const std::string& message, SVal&& data) const;
	virtual SObj error(const std::string& message) const
	{
		return error(message, SVal());
	}

	inline auto requestId() const
	{
		return _requestId;
	}

	inline auto lastConfirmedEvent() const
	{
		return _lastConfirmedEvent;
	}

	inline auto lastConfirmedResponse() const
	{
		return _lastConfirmedResponse;
	}
};

#include "ActionFactory.hpp"

#define REGISTER_ACTION(Type,ActionName) const Dummy ActionName::__dummy = \
	ActionFactory::reg(                                                                         \
		#Type,                                                                                  \
		ActionName::create                                                                      \
	);

#define DECLARE_ACTION(ActionName) \
public:                                                                                         \
    ActionName() = delete;                                                                      \
	ActionName(const ActionName&) = delete;                                                     \
	ActionName& operator=(ActionName const&) = delete;                                          \
	ActionName(ActionName&&) noexcept = delete;                                                 \
	ActionName& operator=(ActionName&&) noexcept = delete;                                      \
                                                                                                \
private:                                                                                        \
    ActionName(                                                                                 \
		const std::shared_ptr<::ServicePart>& servicePart,                                      \
		const std::shared_ptr<Context>& context,                                                \
		const SVal& input                                                                       \
	)                                                                                           \
    : Action(servicePart, context, input)                                                       \
    {};                                                                                         \
                                                                                                \
public:                                                                                         \
    ~ActionName() override = default;                                                           \
                                                                                                \
private:                                                                                        \
    bool validate() override;                                                                   \
    bool execute() override;                                                                    \
                                                                                                \
    static auto create(                                                                         \
		const std::shared_ptr<::ServicePart>& servicePart,                                      \
		const std::shared_ptr<Context>& context,                                                \
		const SVal& input                                                                       \
	)                                                                                           \
    {                                                                                           \
        return std::shared_ptr<Action>(new ActionName(servicePart, context, input));            \
    }                                                                                           \
    static const Dummy __dummy;
