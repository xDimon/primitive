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

Action::Action(
	const std::shared_ptr<Service>& service,
	const std::shared_ptr<Context>& context,
	const SVal* input_
)
: _service(service)
, _context(context)
, _data(nullptr)
, _requestId(0)
, _lastConfirmedEvent(0)
, _lastConfirmedResponse(0)
, _answerSent(false)
{
	auto input = dynamic_cast<const SObj *>(input_);

	if (input == nullptr)
	{
		throw std::runtime_error("Input data isn't object");
	}

	auto reqName = dynamic_cast<const SStr*>(input->get("request"));
	if (reqName != nullptr)
	{
		_actionName = reqName->value();
	}
	else
	{
		int status;
		_actionName = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	}

	auto aux = dynamic_cast<const SObj*>(input->get("_"));
	if (aux != nullptr)
	{
		_requestId = aux->getAsInt("ri");
		_lastConfirmedResponse = aux->getAsInt("cr");
		_lastConfirmedEvent = aux->getAsInt("ce");
	}

	_data = dynamic_cast<const SVal*>(input->get("data"));
}

Action::~Action()
{
	_requestCount++;
}

const SObj* Action::response(const SVal* data) const
{
	auto response = std::make_unique<SObj>();

	if (_requestId != 0)
	{
		auto aux = std::make_unique<SObj>();
		aux->insert("ri", _requestId);
		response->insert("_", aux.release());
	}

	response->insert("response", _actionName);

	if (data != nullptr)
	{
		response->insert("data", data);
	}

	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent");
	}

	_answerSent = true;

	return response.release();
}

const SObj* Action::error(const std::string& message, const SVal* data) const
{
	auto error = std::make_unique<SObj>();

	if (_requestId != 0)
	{
		auto aux = std::make_unique<SObj>();
		aux->insert("ri", _requestId);
		error->insert("_", aux.release());
	}

	error->insert("error", _actionName);

	error->insert("message", message);

	if (data != nullptr)
	{
		error->insert("data", data);
	}

	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent");
	}

	_answerSent = true;

	return error.release();
}
