// Copyright © 2018 Dmitriy Khaustov
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
// File created on: 2018.01.23

// LpsContext.hpp


#pragma once

#include <queue>
#include "../utils/Context.hpp"
#include "../sessions/Session.hpp"
#include "../serialization/SArr.hpp"
#include "TransportContext.hpp"

class LpsContext final: public Context
{
private:
	std::recursive_mutex _mutex;
	std::weak_ptr<ServicePart> _service;
	std::shared_ptr<TransportContext> _context;
	std::weak_ptr<Session> _session;
	std::queue<SVal> _output;
	std::shared_ptr<Timeout> _timeout;
	bool _aggregation;
	bool _compression;
	bool _close;
	bool _closed;

public:
	LpsContext() = delete; // Default-constructor
	LpsContext(const LpsContext&) = delete; // Copy-constructor
	LpsContext& operator=(LpsContext const&) = delete; // Copy-assignment
	LpsContext(LpsContext&&) noexcept = delete; // Move-constructor
	LpsContext& operator=(LpsContext&&) noexcept = delete; // Move-assignment

	LpsContext(
		const std::shared_ptr<ServicePart>& servicePart,
		const std::shared_ptr<TransportContext>& context,
		bool aggregation = false,
		bool compression = false
	);

	virtual ~LpsContext() override;

	void assignContext(const std::shared_ptr<TransportContext>& context)
	{
		_context = context;
	}
	std::shared_ptr<TransportContext> getContext() const
	{
		return _context;
	}

	void assignSession(const std::shared_ptr<Session>& session);
	std::shared_ptr<Session> getSession();
	void resetSession();

	SVal recv();

	void send();

	void out(SVal value, bool close = false);
};
