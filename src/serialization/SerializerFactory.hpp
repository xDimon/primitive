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

// SerializerFactory.hpp


#pragma once

#include "Serializer.hpp"

#include "../configs/Setting.hpp"

#include <memory>
#include <map>
#include <functional>
#include "../utils/Dummy.hpp"

class SerializerFactory final
{
public:
	SerializerFactory(const SerializerFactory&) = delete;
	SerializerFactory& operator=(const SerializerFactory&) = delete;
	SerializerFactory(SerializerFactory&&) noexcept = delete;
	SerializerFactory& operator=(SerializerFactory&&) noexcept = delete;

private:
	SerializerFactory() = default;

	static SerializerFactory& getInstance()
	{
		static SerializerFactory instance;
		return instance;
	}

	std::map<std::string, std::function<std::shared_ptr<Serializer> (uint32_t flags)>> _builders;

public:
	static Dummy reg(const std::string& type, std::function<std::shared_ptr<Serializer> (uint32_t flags)>&& builder) noexcept;
	static std::shared_ptr<Serializer> create(const std::string& type, uint32_t flags = 0);
};
