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
// File created on: 2017.05.31

// Action.cpp


#include "Action.hpp"
#include "../utils/NopeException.hpp"
#include "../telemetry/TelemetryManager.hpp"

Action::Action(
	const std::shared_ptr<ServicePart>& servicePart,
	std::shared_ptr<Context> context,
	const SVal& input_
)
: _servicePart(servicePart)
, _context(std::move(context))
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

	if (!input_.is<SObj>())
	{
		throw std::runtime_error("Input data isn't object");
	}
	auto input = input_.as<SObj>();

	if (input.get("request").is<SStr>())
	{
		_actionName = input.getAs<SStr>("request").value();
	}

	if (input.has("_"))
	{
		auto& aux = input.getAs<SObj>("_");

		aux.trylookup("ri", _requestId);
		aux.trylookup("cr", _lastConfirmedResponse);
		aux.trylookup("ce", _lastConfirmedEvent);
	}

	if (input.has("data"))
	{
		_data = input.extract("data");
	}
}

SObj Action::response(SVal&& data) const
{
	SObj response;

	if (_requestId != 0)
	{
		SObj aux;
		aux.emplace("ri", _requestId);
		response.emplace("_", std::move(aux));
	}

	response.emplace("response", _actionName);

	if (!data.isUndefined())
	{
		response.emplace("data", std::move(data));
	}

	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent (try to send response)");
	}

	_answerSent = true;

	return response;
}

SObj Action::error(const std::string& message, SVal&& data) const
{
	SObj error;

	if (_requestId != 0)
	{
		SObj aux;
		aux.emplace("ri", _requestId);
		error.emplace("_", std::move(aux));
	}

	error.emplace("error", _actionName);

	error.emplace("messages", message);

	if (!data.isUndefined())
	{
		error.emplace("data", std::move(data));
	}

	if (_answerSent)
	{
		throw std::runtime_error("Internal error ← Answer already sent ← Try to send error '" + message + "'");
	}

	_answerSent = true;

	return error;
}

bool Action::doIt(const std::string& where, std::chrono::steady_clock::time_point beginExecTime)
{
	TelemetryManager::metric(where + "/" + _actionName + "/count", 1)->addValue();
	TelemetryManager::metric(where + "/" + _actionName + "/avg_per_sec", std::chrono::seconds(15))->addValue();

	// Подготовка
	try
	{
		prepare();
	}
	catch (const std::exception& exception)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/invalid", 1)->addValue();
		throw std::runtime_error(std::string() + "Fail preparing action for doing ← " + exception.what());
	}

	// Валидация
	try
	{
		validate();
	}
	catch (const std::exception& exception)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/invalid", 1)->addValue();
		throw std::runtime_error(std::string() + "Fail validation of action data ← " + exception.what());
	}

	// Выполнение
	try
	{
		execute();
	}
	catch (const NopeException& exception)
	{
		throw;
	}
	catch (const std::exception& exception)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/fail", 1)->addValue();
		throw std::runtime_error(std::string() + "Fail execute of action ← " + exception.what());
	}

	TelemetryManager::metric(where + "/" + _actionName + "/success", 1)->addValue();

	auto now = std::chrono::steady_clock::now();
	auto timeSpent =
		static_cast<double>(std::chrono::steady_clock::duration(now - beginExecTime).count()) /
		static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
	if (timeSpent > 0)
	{
		TelemetryManager::metric(where + "/" + _actionName + "/avg_exec_time", std::chrono::seconds(15))->addValue(timeSpent, now);
	}

	return true;
}
