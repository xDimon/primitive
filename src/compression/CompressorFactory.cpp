// Copyright © 2018-2019 Dmitriy Khaustov
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
// File created on: 2018.07.07

// CompressorFactory.cpp


#include "CompressorFactory.hpp"

#include <iostream>

Dummy CompressorFactory::reg(const std::string& type, std::shared_ptr<Compressor> (* creator)(uint32_t flags)) noexcept
{
	auto& factory = getInstance();

	auto i = factory._creators.find(type);
	if (i != factory._creators.end())
	{
		std::cerr << "Internal error: Attepmt to register compressor with the same type (" << type << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
	factory._creators.emplace(type, creator);
	return Dummy{};
}

std::shared_ptr<Compressor> CompressorFactory::create(const std::string& type, uint32_t flags)
{
	auto& factory = getInstance();

	auto i = factory._creators.find(type);
	if (i == factory._creators.end())
	{
		throw std::runtime_error("Unknown compressor type");
	}
	return std::move(i->second(flags));
}
