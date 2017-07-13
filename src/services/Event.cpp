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

// Event.cpp


#include "Event.hpp"

#include <cxxabi.h>

Event::Id Event::_eventCount = 0;

Event::Event(const std::shared_ptr<Context>& context, const SVal* data)
: _context(context)
, _data(data)
, _eventId(0)
{
	_name = nullptr;
}

Event::~Event()
{
}

const SObj* Event::event()
{
	auto event = std::make_unique<SObj>();

	_eventId = ++_eventCount;

	auto aux = std::make_unique<SObj>();
	aux->insert("ei", static_cast<SInt::type>(_eventId));
	event->insert("_", aux.release());

	event->insert("event", getName());

	if (_data)
	{
		event->insert("data", _data);
	}

	return event.release();
}

const char* Event::getName() const
{
	if (!_name)
	{
		int status;
		_name = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	}
	return _name;
}
