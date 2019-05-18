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
// File created on: 2017.08.16

// Sink.hpp


#pragma once

#include <mutex>
#include "../configs/Setting.hpp"
#include "Log.hpp"
#include "../utils/Timer.hpp"

#define PRE_ACCUMULATE_LOG

class Sink final: public Shareable<Sink>
{
public:
	enum class Type : uint8_t {
		BLACKHOLE = 0,
		CONSOLE,
		FILE
	};

private:
	using mutex_t = std::mutex;
	mutex_t _mutex;
	std::string _name;
	Type _type;

	static const size_t accumucatorCapacity;
	std::string _accumulator;
	std::shared_ptr<Timer> _flushTimeout;

	FILE* _f;
	std::string _path;
	std::string _preInitBuff;

public:
	Sink(const Sink&) = delete; // Copy-constructor
	Sink& operator=(const Sink&) = delete; // Copy-assignment
	Sink(Sink&&) noexcept = delete; // Move-constructor
	Sink& operator=(Sink&&) noexcept = delete; // Move-assignment

	Sink();
	explicit Sink(const Setting& setting);
	~Sink() override;

	const std::string& name() const
	{
		return _name;
	}

	void push(Log::Detail level, const std::string& name, const std::string& message);
	void push(Log::Detail level, const std::string& name, const std::string& format, va_list ap);

	void flush();
	void rotate();
};
