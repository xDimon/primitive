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
// File created on: 2017.04.06

// Session.hpp


#pragma once

#include <memory>
#include "../utils/Shareable.hpp"
#include "../utils/Context.hpp"
#include "../services/Service.hpp"

class Session : public Shareable<Session>
{
public:
	typedef uint64_t HID;
	typedef uint64_t SID;

protected:
	static const char _hidSeed[16];

protected:
	std::shared_ptr<Service> _service;

public:
	const HID hid;

protected:
	SID _sid;
	std::weak_ptr<Context> _context;
	bool _ready;

public:
	Session(
		const std::shared_ptr<Service>& service,
		HID hid
	);
	virtual ~Session();

	const bool isReady() const
	{
		return _ready;
	}

	const SID sid() const
	{
		return _sid;
	}
	void setSid(SID sid);

	void assignContext(const std::shared_ptr<Context>& context)
	{
		_context = context;
	}
	std::shared_ptr<Context> getContext() const
	{
		return _context.lock();
	}
};
