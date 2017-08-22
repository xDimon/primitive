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
// File created on: 2017.05.31

// Action.hpp


#pragma once


#include <cstdint>
#include "../utils/Context.hpp"
#include "../serialization/SVal.hpp"
#include "../serialization/SObj.hpp"
#include "../transport/ServerTransport.hpp"
#include "Service.hpp"

class Action
{
protected:
	using Id = int64_t;

	std::string _actionName;

	static uint64_t _requestCount;
	std::shared_ptr<Service> _service;
	std::shared_ptr<Context> _context;
	const SVal* _data;
	Id _requestId;
	Id _lastConfirmedEvent;
	Id _lastConfirmedResponse;
	mutable bool _answerSent;

public:
	Action() = delete;
	Action(const Action&) = delete;
	void operator=(Action const&) = delete;
	Action(Action&&) = delete;
	Action& operator=(Action&&) = delete;

	Action(
		const std::shared_ptr<Service>& service,
		const std::shared_ptr<Context>& context,
		const SVal* input
	);
	virtual ~Action();

	static inline auto getCount()
	{
		return _requestCount;
	}

	virtual bool validate() = 0;

	virtual bool execute() = 0;

	virtual const SObj* response(const SVal* data) const;
	inline const SObj* response() const
	{
		return response(nullptr);
	}

	virtual const SObj* error(const std::string& message, const SVal* data) const;
	virtual const SObj* error(const std::string& message) const
	{
		return error(message, nullptr);
	}

	inline auto requestId() const
	{
		return _requestId;
	}

	inline auto lastConfirmedMessage() const
	{
		return _lastConfirmedEvent;
	}

	inline auto lastConfirmedResponse() const
	{
		return _lastConfirmedResponse;
	}
};

#include "ActionFactory.hpp"

#define REGISTER_ACTION(Type,ActionName) const bool ActionName::__dummy = ActionFactory::reg(#Type, ActionName::create);
#define DECLARE_ACTION(ActionName) \
public:                                                                                         \
    ActionName() = delete;                                                                      \
	ActionName(const ActionName&) = delete;                                                     \
	void operator=(ActionName const&) = delete;                                                 \
	ActionName(ActionName&&) = delete;                                                          \
	ActionName& operator=(ActionName&&) = delete;                                               \
                                                                                                \
private:                                                                                        \
    ActionName(                                                                                 \
		const std::shared_ptr<::Service>& service,                                              \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input                                                                       \
	)                                                                                           \
    : Action(service, context, input)                                                           \
    {};                                                                                         \
                                                                                                \
public:                                                                                         \
    ~ActionName() override = default;                                                           \
                                                                                                \
    bool validate() override;                                                                   \
    bool execute() override;                                                                    \
                                                                                                \
private:                                                                                        \
    static auto create(                                                                         \
		const std::shared_ptr<::Service>& service,                                              \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input                                                                       \
	)                                                                                           \
    {                                                                                           \
        return std::shared_ptr<Action>(new ActionName(service, context, input));                \
    }                                                                                           \
    static const bool __dummy;
