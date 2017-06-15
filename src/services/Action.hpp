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

class Action
{
private:
	mutable const char* _name;

protected:
	using Id = uint32_t;

	static uint64_t _requestCount;
	std::shared_ptr<Context> _context;
	const SVal* _input;
	const SObj* _data;
	ServerTransport::Transmitter _transmitter;
	Id _requestId;
	Id _lastConfirmedEvent;
	Id _lastConfirmedResponse;

public:
	Action() = delete;
	Action(const Action&) = delete;
	void operator=(Action const&) = delete;

	Action(
		const std::shared_ptr<Context>& context,
		const SVal* input,
		ServerTransport::Transmitter _transmitter
	);
	virtual ~Action();

	static inline auto getCount()
	{
		return _requestCount;
	}

	const char* getName() const;

	virtual bool validate() = 0;

	virtual bool execute() = 0;

	virtual bool isCanBeFirst() const
	{
		return false;
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

#define REGISTER_ACTION(Type,ActionName) const bool ActionName::__dummy_for_reg_call = ActionFactory::reg(#Type, ActionName::create);
#define DECLARE_ACTION(ActionName) \
private:                                                                                        \
    ActionName() = delete;                                                                      \
    ActionName(const ActionName&) = delete;                                                     \
    void operator= (ActionName const&) = delete;                                                \
                                                                                                \
    ActionName(                                                                                 \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input,                                                                      \
		ServerTransport::Transmitter transmitter                                                      \
	)                                                                                           \
    : Action(context, input, transmitter)                                                       \
    {};                                                                                         \
                                                                                                \
public:                                                                                         \
    virtual ~ActionName() {};                                                                   \
                                                                                                \
    virtual bool validate() override;                                                           \
    virtual bool execute() override;                                                            \
                                                                                                \
private:                                                                                        \
    static auto create(                                                                         \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input,                                                                      \
		ServerTransport::Transmitter transmitter                                                      \
	)                                                                                           \
    {                                                                                           \
        return std::shared_ptr<Action>(new ActionName(context, input, transmitter));            \
    }                                                                                           \
    static const bool __dummy_for_reg_call;
