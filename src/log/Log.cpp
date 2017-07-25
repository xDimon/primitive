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

Log::Log(const std::string& name, Detail detail)
: _detail(detail)
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

	if (_detail == Detail::UNDEFINED)
	{
		_detail = LoggerManager::defaultLogLevel();
	}
}

void Log::setDetail(Detail detail)
{
	_detail = detail;
}

void Log::setName(const std::string& name_)
{
	auto name = name_;
	name.resize(18, ' ');

	if (_tracer)
	{
		_tracer->Register_Module(name.c_str(), &module);
	}
}
