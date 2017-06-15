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

// Action.cpp


#include <cxxabi.h>
#include "Action.hpp"
#include "../serialization/SObj.hpp"

uint64_t Action::_requestCount = 0;

Action::Action(const std::shared_ptr<Context>& context, const SVal* input, ServerTransport::Transmitter transmitter)
: _context(context)
, _input(input)
, _data(dynamic_cast<const SObj *>(_input))
, _transmitter(transmitter)
, _requestId(0)
, _lastConfirmedEvent(0)
, _lastConfirmedResponse(0)
{
	_name = nullptr;

	if (_data)
	{
		auto aux = dynamic_cast<const SObj*>(_data->get("_"));
		if (aux)
		{
//			_requestId = aux->get("ri"); // request id
//			_lastConfirmedResponse = aux->get("cr"); // confirmed response id
//			_lastConfirmedEvent = aux->get("ce"); // confirmed event id
		}
	}
}

Action::~Action()
{
	_requestCount++;
}

const char* Action::getName() const
{
	int status;
	return _name ?: (_name = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status));
}
