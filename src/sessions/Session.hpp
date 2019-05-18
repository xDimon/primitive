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
// File created on: 2017.04.06

// Session.hpp


#pragma once

#include <memory>
#include <chrono>
#include "../utils/Shareable.hpp"
#include "../utils/Context.hpp"
#include "../services/Service.hpp"
#include "../utils/Timer.hpp"
#include "Communicate.hpp"

class Session : public Shareable<Session>, public Communicate
{
public:
	typedef int64_t HID;
	typedef std::string SID;

protected:
	static const char _hidSeed[16];
	mutable std::recursive_mutex _mutex;

protected:
	std::shared_ptr<Service> _service;

	[[deprecated]] virtual std::chrono::seconds saveDuration() const
	{
		return delayBeforeSaving();
	}

	[[deprecated]] virtual std::chrono::seconds timeoutDuration() const
	{
		return delayBeforeUnload(false);
	}

	virtual std::chrono::seconds delayBeforeSaving() const
	{
		return std::chrono::seconds(60);
	}

	virtual std::chrono::seconds delayBeforeUnload(bool isShort) const
	{
		return isShort ? std::chrono::seconds(15) : std::chrono::seconds(900);
	}

	/// Таймаут сохранения
	std::shared_ptr<Timer> _timeoutForSave;

	/// Таймаут выгрузки
	std::shared_ptr<Timer> _timeoutForUnload;

protected:
	SID _sid;
	bool _ready;

public:
	Session() = delete;
	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) noexcept = delete;
	Session& operator=(Session&&) noexcept = delete;

	explicit Session(const std::shared_ptr<Service>& service);

	[[deprecated]]
	Session(
		const std::shared_ptr<Service>& service,
		HID hid
	)
	: Session(service) {};

	~Session() override = default;

	virtual void changed();
	void touch(bool temporary = false);

	const bool isReady() const
	{
		return _ready;
	}

	SID sid() const
	{
		return _sid;
	}
	void setSid(SID sid);

	virtual bool load();
	virtual bool save();
	virtual void close(const std::string& reason);

	void protectedDo(const std::function<void()>& func)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		func();
	}
};
