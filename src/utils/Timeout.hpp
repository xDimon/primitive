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
// File created on: 2017.09.18

// Timeout.hpp


#pragma once

#include <functional>
#include <mutex>
#include "TimeoutWatcher.hpp"

class Timeout final: public Shareable<Timeout>
{
private:
	friend class TimeoutWatcher;

	std::recursive_mutex _mutex;

	std::chrono::steady_clock::time_point realExpireTime;
	std::chrono::steady_clock::time_point nextExpireTime;
	std::function<void()> _alarmHandler;
	const char *_label;

	std::shared_ptr<TimeoutWatcher> _timeoutWatcher;

public:
	Timeout(const Timeout&) = delete; // Copy-constructor
	Timeout& operator=(Timeout const&) = delete; // Copy-assignment
	Timeout(Timeout&&) noexcept = delete; // Move-constructor
	Timeout& operator=(Timeout&&) noexcept = delete; // Move-assignment

	explicit Timeout(std::function<void()> handler, const char* label/* = "Timeout"*/);
	~Timeout() override = default;

	// Метка задачи (иня, название и т.п., для отладки)
 	const char* label() const
	{
		return _label;
	}

	bool operator()();

	void startOnce(std::chrono::milliseconds duration);
	void restart(std::chrono::milliseconds duration);
};
