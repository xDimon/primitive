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

class Timeout: public Shareable<Timeout>
{
private:
	friend class TimeoutWatcher;

	std::mutex _mutex;

	std::chrono::steady_clock::time_point realExpireTime;
	std::chrono::steady_clock::time_point nextExpireTime;
	std::function<void()> _alarmHandler;

	std::shared_ptr<TimeoutWatcher> _timeoutWatcher;

public:
	Timeout(const Timeout&) = delete; // Copy-constructor
	void operator=(Timeout const&) = delete; // Copy-assignment
	Timeout(Timeout&&) = delete; // Move-constructor
	Timeout& operator=(Timeout&&) = delete; // Move-assignment

	explicit Timeout(std::function<void()> handler);
	~Timeout() override = default;

	bool operator()();

	void startOnce(std::chrono::milliseconds duration);
	void restart(std::chrono::milliseconds duration);
};
