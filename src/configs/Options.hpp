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

// Options.hpp


#pragma once

#include <memory>
#include <string>
#include "../../lib/cxxopts/include/cxxopts.hpp"

class Options: protected cxxopts::Options
{
private:
	std::string _configFile;

public:
	Options() = delete;
	Options(const Options&) = delete;
	Options& operator=(const Options&) = delete;
	Options(Options&&) noexcept = delete;
	Options& operator=(Options&&) noexcept = delete;

	Options(int argc, char *argv[]);
	~Options() = default;

	const std::string& configFile() const
	{
		return _configFile;
	}
};
