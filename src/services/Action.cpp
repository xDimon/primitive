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

uint64_t Action::_requestCount = 0;

Action::Action(const std::shared_ptr<Context>& context, const SVal* input, ServerTransport::Transmitter transmitter)
: _context(context)
, _input(dynamic_cast<const SObj *>(input))
, _data(nullptr)
, _transmitter(transmitter)
, _requestId(0)
, _lastConfirmedEvent(0)
, _lastConfirmedResponse(0)
, _answerSent(false)
{
	_name = nullptr;

	if (!_input)
	{
		throw std::runtime_error("Input data isn't object");
	}

	auto aux = dynamic_cast<const SObj*>(_input->get("_"));
	if (aux)
	{
		_requestId = aux->getAsInt("ri");
		_lastConfirmedResponse = aux->getAsInt("cr");
		_lastConfirmedEvent = aux->getAsInt("ce");
	}

	_data = dynamic_cast<const SVal*>(_input->get("data"));
}

Action::~Action()
{
	_requestCount++;
}

const SObj* Action::response(const SVal* data) const
{
	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent");
	}

	auto response = std::make_unique<SObj>();

	if (_requestId)
	{
		auto aux = std::make_unique<SObj>();
		aux->insert("ri", _requestId);
		response->insert("_", aux.release());
	}

	response->insert("response", getName());

	if (data)
	{
		response->insert("data", data);
	}

	_answerSent = true;

	return response.release();
}

const SObj* Action::error(const std::string& message, const SVal* data) const
{
	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent");
	}

	auto error = std::make_unique<SObj>();

	if (_requestId)
	{
		auto aux = std::make_unique<SObj>();
		aux->insert("ri", _requestId);
		error->insert("_", aux.release());
	}

	error->insert("error", getName());

	error->insert("message", message);

	if (data)
	{
		error->insert("data", data);
	}

	_answerSent = true;

	return error.release();
}

const char* Action::getName() const
{
	if (!_name)
	{
		int status;
		auto reqName = _input ? dynamic_cast<const SStr*>(_input->get("request")) : nullptr;
		_name = reqName ?
				reqName->value().c_str() :
				abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	}
	return _name;
}
