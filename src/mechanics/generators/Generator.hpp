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
// File created on: 2017.09.21

// Generator.hpp


#pragma once

#include <memory>
#include <chrono>
#include <mutex>
#include "../../utils/Time.hpp"
#include "../../serialization/SObj.hpp"

class GeneratorConfig;

class Generator
{
public:
	typedef std::string Id;
	typedef uint32_t Period;

	const Id id;

private:
	std::shared_ptr<const GeneratorConfig> _config;
	std::mutex _mutex;

	Time::Timestamp _nextTick;
	bool _changed;

public:
	Generator() = delete;

	Generator(const Generator&) = delete; // Copy-constructor
	void operator=(Generator const&) = delete; // Copy-assignment
	Generator(Generator&&) = delete; // Move-constructor
	Generator& operator=(Generator&&) = delete; // Move-assignment

	explicit Generator(const Id& id);

	Generator(const Id& id, Time::Timestamp nextTick);

	virtual ~Generator() = default;

	inline Time::Timestamp nextTick() const
	{
		return _nextTick;
	}

	bool start();
	bool stop();

	bool tick();

	bool inDefaultState() const;

	void setChanged(bool isChanged = true);
	inline bool isChanged() const
	{
		return _changed;
	}

	SObj* serialize() const;
};
