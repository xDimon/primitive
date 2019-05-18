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

// Communicate.hpp


#pragma once


#include <cstdint>
#include <chrono>
#include "../serialization/SVal.hpp"
#include "../utils/Context.hpp"
#include "../services/Action.hpp"

class Communicate
{
private:
	mutable std::recursive_mutex _mutex;

	// Id последнего события
	int64_t _lastEventId;

	/// Id текущего реквеста
	int64_t _requestId;

	/// Сщбытия, получение которых еще не подтверждено клиентом
	std::map<int64_t, const SVal> _unconfirmedEvents;

	/// Ответы, получение которых еще не подтверждено клиентом
	std::map<int64_t, const SVal> _unconfirmedResponses;

protected:
	std::shared_ptr<Context> _context;

public:
	Communicate(Communicate&&) noexcept = delete; // Move-constructor
	Communicate(const Communicate&) = delete; // Copy-constructor
	~Communicate() = default; // Destructor
	Communicate& operator=(Communicate&&) noexcept = delete; // Move-assignment
	Communicate& operator=(const Communicate&) = delete; // Copy-assignment

	Communicate()
	: _lastEventId(0)
	, _requestId(0)
	{}

	void assignContext(const std::shared_ptr<Context>& context)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		_context = context;
	}
	virtual std::shared_ptr<Context> getContext()
	{
		return _context;
	}

	void sendEvent(const std::string& name, SVal&& data = SVal(), bool close = false);
	void sendResponse(const Action& action, SVal&& data = SVal());
	void sendError(const Action& action, const std::string& message = "", SVal&& data = SVal());

	void confirmEvents(int64_t id);
	void resendEvents();

	void confirmResponse(int64_t id);
	bool resendResponse(int64_t id);

	void setRequestId(int64_t id);

	void resetCaches();
};
