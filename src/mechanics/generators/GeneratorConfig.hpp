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

// GeneratorConfig.hpp


#pragma once


#include "Generator.hpp"

class GeneratorConfig
{
public:
	const Generator::Id id;
	const Generator::Period period;

public:
	GeneratorConfig(const GeneratorConfig&) = delete; // Copy-constructor
	GeneratorConfig& operator=(GeneratorConfig const&) = delete; // Copy-assignment
	GeneratorConfig(GeneratorConfig&&) = delete; // Move-constructor
	GeneratorConfig& operator=(GeneratorConfig&&) = delete; // Move-assignment

	GeneratorConfig(Generator::Id id, Generator::Period period);

	~GeneratorConfig() = default;
};
