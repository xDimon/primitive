// Copyright © 2017 Dmitriy Khaustov
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
// File created on: 2017.03.11

// Log.cpp


#include "Log.hpp"
#include "LoggerManager.hpp"

Log::Log(std::string name)
{
	if (LoggerManager::enabled)
	{
		_tracer.reset(LoggerManager::getLogTrace());
		if (!_tracer)
		{
			throw std::runtime_error("Can't get shared p7-channel");
		}
	}

	module = nullptr;

	setName(name);
}

Log::Log()
: Log("-")
{
}

Log::~Log()
{
}

void Log::setName(std::string name)
{
	name.resize(18, ' ');

	if (_tracer)
	{
		_tracer->Register_Module(name.c_str(), &module);
	}
}
