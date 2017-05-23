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
// File created on: 2017.05.23

// Task.hpp


#pragma once


#include <functional>
#include <chrono>

class Task
{
public:
	using Func = std::function<void()>;
	using Clock = std::chrono::steady_clock;
	using Duration = Clock::duration;
	using Time = Clock::time_point;

private:
	Func _function;
	Time _until;
	bool _immediately;

	Task(Task const &) = delete;
	void operator=(Task const &) = delete;

public:
	Task();
	explicit Task(Func function);
	Task(Func function, Duration delay);
	Task(Func function, Time time);
	Task(Task const &&that);

	virtual ~Task() {}

	void operator=(Task const &&that);

	bool operator<(const Task &that) const;

	bool operator()();

	const Time& until() const
	{
		return _until;
	}
};
