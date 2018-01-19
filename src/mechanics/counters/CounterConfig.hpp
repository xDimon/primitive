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
// File created on: 2017.09.20

// CounterConfig.hpp


#pragma once


#include "Counter.hpp"

class CounterConfig
{
public:
	const Counter::Id id; // Id конфига

public:
	CounterConfig(const CounterConfig&) = delete; // Copy-constructor
	CounterConfig& operator=(CounterConfig const&) = delete; // Copy-assignment
	CounterConfig(CounterConfig&&) = delete; // Move-constructor
	CounterConfig& operator=(CounterConfig&&) = delete; // Move-assignment

	CounterConfig(Counter::Id id);

	~CounterConfig() = default;
};
