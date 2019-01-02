// Copyright © 2018-2019 Dmitriy Khaustov
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
// File created on: 2018.05.14

// Communicate.cpp


#include "Communicate.hpp"
#include "../transport/LpsContext.hpp"

void Communicate::sendEvent(const std::string& name, SVal&& data, bool close)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	SObj event;
	event.emplace("event", name);

	if (!data.isUndefined())
	{
		event.emplace("data", std::move(data));
	}

	if (!close)
	{
		int64_t id = ++_lastEventId;

		SObj aux;
		aux.emplace("ei", id);
		event.emplace("_", std::move(aux));

		_unconfirmedEvents.emplace(id, event);
	}

	auto context = std::dynamic_pointer_cast<LpsContext>(getContext());
	if (context)
	{
		context->out(std::move(event), close);
	}
}

void Communicate::sendResponse(const Action& action, SVal&& data)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto&& response = action.response(std::move(data));

	if (_requestId)
	{
		auto i = _unconfirmedResponses.find(_requestId);
		if (i != _unconfirmedResponses.end())
		{
			auto cachedResponse = i->second;
			_unconfirmedResponses.erase(i);
		}

		_unconfirmedResponses.emplace(_requestId, response);
	}
	_requestId = 0;

	auto context = std::dynamic_pointer_cast<LpsContext>(getContext());
	if (context)
	{
		context->out(std::move(response));
	}
}

void Communicate::sendError(const Action& action, const std::string& message, SVal&& data)
{
	auto context = std::dynamic_pointer_cast<LpsContext>(getContext());
	if (!context)
	{
		return;
	}

	auto&& error = action.error(message, std::move(data));

	context->out(std::move(error));
}

void Communicate::confirmEvents(int64_t id)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	_unconfirmedEvents.erase(_unconfirmedEvents.begin(), _unconfirmedEvents.upper_bound(id));
}

void Communicate::resendEvents()
{
	auto context = std::dynamic_pointer_cast<LpsContext>(getContext());
	if (!context)
	{
		return;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	for (auto i : _unconfirmedEvents)
	{
		context->out(i.second);
	}
}

void Communicate::confirmResponse(int64_t id)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (id)
	{
		auto i = _unconfirmedResponses.find(id);
		if (i != _unconfirmedResponses.end())
		{
			_unconfirmedResponses.erase(i);
		}
	}
	else
	{
		_unconfirmedResponses.clear();
	}
}

bool Communicate::resendResponse(int64_t id)
{
	if (id == 0)
	{
		return false;
	}

	auto context = std::dynamic_pointer_cast<LpsContext>(getContext());
	if (!context)
	{
		return false;
	}

	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto i = _unconfirmedResponses.find(id);
	if (i != _unconfirmedResponses.end())
	{
		context->out(i->second);
		return true;
	}

	return false;
}

void Communicate::setRequestId(int64_t id)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (id != 0 && _requestId != 0)
	{
		throw std::runtime_error("Race condition of requests");
	}

	_requestId = id;
}

void Communicate::resetCaches()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	_requestId = 0;
	_unconfirmedResponses.clear();
	_unconfirmedEvents.clear();
}
