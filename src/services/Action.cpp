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
#include "../utils/NopeException.hpp"
#include "../telemetry/TelemetryManager.hpp"

Action::Action(
	const std::shared_ptr<ServicePart>& servicePart,
	const std::shared_ptr<Context>& context,
	const SVal* input_
)
: _servicePart(servicePart)
, _context(context)
, _data(nullptr)
, _requestId(0)
, _lastConfirmedEvent(0)
, _lastConfirmedResponse(0)
, _answerSent(false)
{
	if (!_servicePart)
	{
		throw std::runtime_error("Internal error ← Bad service part");
	}

	_service = servicePart->service<::Service>();
	if (!_service)
	{
		throw std::runtime_error("Internal error ← Bad service");
	}

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
		_actionName = abi::__cxa_demangle(typeid(*this).name(), nullptr, nullptr, &status);
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

bool Action::doIt(const std::string& where, std::chrono::steady_clock::time_point beginExecTime)
{
	bool throwNope = false;

	TelemetryManager::metric(where + "/" + _actionName + "/count", 1)->addValue();
	TelemetryManager::metric(where + "/" + _actionName + "/avg_per_sec", std::chrono::seconds(15))->addValue();

	// Валидация
	try
	{
		if (!validate())
		{
			throw std::runtime_error("Data isn't valid");
		}
	}
	catch (const std::exception& exception)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/invalid", 1)->addValue();
		throw std::runtime_error(std::string() + "Fail validation of action data ← " + exception.what());
	}

	// Выполнение
	try
	{
		if (!execute())
		{
			throw std::runtime_error("Fail execute of action");
		}
	}
	catch (const NopeException& exception)
	{
		throwNope = true;
	}
	catch (const std::exception& exception)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/fail", 1)->addValue();
		throw std::runtime_error(std::string() + "Fail execute of action ← " + exception.what());
	}

	TelemetryManager::metric(where + "/" + _actionName + "/success", 1)->addValue();

	auto now = std::chrono::steady_clock::now();
	auto timeSpent = static_cast<double>((now - beginExecTime).count()) / static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/avg_exec_time", std::chrono::seconds(15))->addValue(timeSpent, now);
	}

	if (throwNope) throw NopeException{};

	return true;
}
