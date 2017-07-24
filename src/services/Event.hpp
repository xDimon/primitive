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
// File created on: 2017.06.15

// Event.hpp


#pragma once


#include <cstdint>
#include "../utils/Context.hpp"
#include "../serialization/SVal.hpp"
#include "../serialization/SObj.hpp"
#include "../transport/ServerTransport.hpp"

class Event
{
private:
	mutable const char* _name;

protected:
	using Id = int32_t;

	static Id _eventCount;
	std::shared_ptr<Context> _context;
	const SVal* _data;
	Id _eventId;

public:
	Event() = delete;
	Event(const Event&) = delete;
	void operator=(Event const&) = delete;

	Event(
		const std::shared_ptr<Context>& context,
		const SVal* input
	);
	virtual ~Event() = default;

	const char* getName() const;

	virtual const SObj* event();
};

#include "EventFactory.hpp"

#define REGISTER_EVENT(Type,EventName) const bool EventName::__dummy_for_reg_call = EventFactory::reg(#Type, EventName::create);
#define DECLARE_EVENT(EventName) \
private:                                                                                        \
    EventName() = delete;                                                                       \
    EventName(const EventName&) = delete;                                                       \
    void operator= (EventName const&) = delete;                                                 \
                                                                                                \
    EventName(                                                                                  \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input                                                                       \
	)                                                                                           \
    : Event(context, input)                                                                     \
    {};                                                                                         \
                                                                                                \
public:                                                                                         \
    ~EventName() override = default;                                                            \
                                                                                                \
private:                                                                                        \
    static auto create(                                                                         \
		const std::shared_ptr<Context>& context,                                                \
		const SVal* input                                                                       \
	)                                                                                           \
    {                                                                                           \
        return std::shared_ptr<Event>(new EventName(context, input));                           \
    }                                                                                           \
    static const bool __dummy_for_reg_call;
