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
// File created on: 2017.06.15

// Event.cpp


#include "Event.hpp"

#include <cxxabi.h>

Event::Id Event::_eventCount = 0;

Event::Event(const std::shared_ptr<Context>& context, SVal data)
: _context(context)
, _data(std::move(data))
, _eventId(0)
{
	_name = nullptr;
}

SObj Event::event()
{
	SObj event;

	_eventId = ++_eventCount;

	SObj aux;
	aux.emplace("ei", static_cast<SInt::type>(_eventId));
	event.emplace("_", std::move(aux));

	event.emplace("event", getName());

	if (!_data.isUndefined())
	{
		event.emplace("data", _data);
	}

	return event;
}

const char* Event::getName() const
{
	if (_name == nullptr)
	{
		int status;
		_name = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
	}
	return _name;
}
