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

// CompressorFactory.hpp


#pragma once

#include "Compressor.hpp"

#include <memory>
#include <map>
#include "../utils/Dummy.hpp"

class CompressorFactory final
{
public:
	CompressorFactory(const CompressorFactory&) = delete;
	CompressorFactory& operator=(CompressorFactory const&) = delete;
	CompressorFactory(CompressorFactory&&) noexcept = delete;
	CompressorFactory& operator=(CompressorFactory&&) noexcept = delete;

private:
	CompressorFactory() = default;

	static CompressorFactory& getInstance()
	{
		static CompressorFactory instance;
		return instance;
	}

	std::map<const std::string, std::shared_ptr<Compressor>(*)(uint32_t)> _creators;

public:
	static Dummy reg(const std::string& type, std::shared_ptr<Compressor>(*)(uint32_t)) noexcept;
	static std::shared_ptr<Compressor> create(const std::string& type, uint32_t flags = 0);
};
