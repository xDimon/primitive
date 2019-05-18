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
// File created on: 2017.04.30

// SerializerFactory.cpp


#include "SerializerFactory.hpp"
#include <iostream>

Dummy SerializerFactory::reg(const std::string& type, std::function<std::shared_ptr<Serializer> (uint32_t flags)>&& builder) noexcept
{
	auto& factory = getInstance();

	auto i = factory._builders.find(type);
	if (i != factory._builders.end())
	{
		std::cerr << "Internal error: Attempt to register serializer with the same type (" << type << ')' << std::endl;
		exit(EXIT_FAILURE);
	}
	factory._builders.emplace(type, std::move(builder));
	return Dummy{};
}

std::shared_ptr<Serializer> SerializerFactory::create(const std::string& type, uint32_t flags)
{
	auto& factory = getInstance();

	auto i = factory._builders.find(type);
	if (i == factory._builders.end())
	{
		throw std::runtime_error("Unknown serializer type");
	}
	return i->second(flags);
}
