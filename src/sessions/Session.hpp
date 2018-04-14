// Copyright © 2017-2018 Dmitriy Khaustov
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
#include <chrono>
#include "../utils/Shareable.hpp"
#include "../utils/Context.hpp"
#include "../services/Service.hpp"
#include "../utils/Timeout.hpp"

class Session : public Shareable<Session>
{
public:
	typedef int64_t HID;
	typedef std::string SID;

protected:
	static const char _hidSeed[16];
	mutable std::recursive_mutex _mutex;

protected:
	std::shared_ptr<Service> _service;

	/// Таймаут сохранения
	virtual std::chrono::seconds saveDuration() const
	{
		return std::chrono::seconds(60);
	}
	std::shared_ptr<Timeout> _timeoutForSave;

	/// Таймаут закрытия
	virtual std::chrono::seconds timeoutDuration() const
	{
		return std::chrono::seconds(900);
	}
	std::shared_ptr<Timeout> _timeoutForClose;

public:
	const HID hid;

protected:
	SID _sid;
	std::shared_ptr<Context> _context;
	bool _ready;

public:
	Session() = delete;
	Session(const Session&) = delete;
	Session& operator=(Session const&) = delete;
	Session(Session&&) noexcept = delete;
	Session& operator=(Session&&) noexcept = delete;

	Session(
		const std::shared_ptr<Service>& service,
		HID hid
	);
	~Session() override = default;

	void changed();
	void touch();

	const bool isReady() const
	{
		return _ready;
	}

	SID sid() const
	{
		return _sid;
	}
	void setSid(SID sid);

	void assignContext(const std::shared_ptr<Context>& context)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		_context = context;
	}
	virtual std::shared_ptr<Context> getContext()
	{
//		return _context.lock();
		return _context;
	}

	virtual bool load();
	virtual bool save();
	virtual void close(const std::string& reason);

	void protectedDo(const std::function<void()>& func)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		func();
	}
};
