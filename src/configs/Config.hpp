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
// File created on: 2017.02.25

// Config.hpp


#pragma once

#include <memory>
#include "Options.hpp"
#include <serialization/SObj.hpp>

class Config final
{
public:
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;
	Config(Config&&) noexcept = delete;
	Config& operator=(Config&&) noexcept = delete;

private:
	Config() = default;
	~Config() = default;

	static Config& getInstance()
	{
		static Config instance;
		return instance;
	}

	std::shared_ptr<Options> _options;
	SObj _settings;

	static SObj read(std::string path);
	static void processIncludes(SObj& config);

public:
	static void init(const std::shared_ptr<Options>& options);

	[[nodiscard]]
	static const SObj& settings()
	{
		return getInstance()._settings;
	}
};
