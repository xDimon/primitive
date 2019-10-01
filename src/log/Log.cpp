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
// File created on: 2017.03.11

// Log.cpp


#include "Log.hpp"
#include <cstdarg>
#include "LoggerManager.hpp"

Log::Log(const std::string& name, Detail detail, const std::string& sink)
: _detail(detail)
{
	if (sink.empty())
	{
		auto s = LoggerManager::getSinkAndLevel(name);
		_sink = std::get<0>(s);
		_detail = std::get<1>(s);
	}
	else
	{
		_sink = LoggerManager::getSink("");
		_detail = Detail::INFO;
	}

	if (detail != Detail::UNDEFINED)
	{
		_detail = detail;
	}

	setName(name);
}

void Log::setDetail(Detail detail)
{
	_detail = detail;
}

void Log::setName(const std::string& name)
{
	_name = name;
	_name.resize(18, ' ');
}

void Log::trace(const std::string& message)
{
	if constexpr (minimalDetalizationLevel <= Detail::TRACE)
	{
		if (_detail == Detail::TRACE)
		{
			_sink->push(Detail::TRACE, _name, message);
		}
	}
}

void Log::trace(const char* fmt, ...)
{
	if constexpr (minimalDetalizationLevel <= Detail::TRACE)
	{
		if (_detail == Detail::TRACE)
		{
			va_list ap;
			va_start(ap, fmt);
			_sink->push(Detail::TRACE, _name, fmt, ap);
		}
	}
}

void Log::debug(const std::string& message)
{
	if constexpr (minimalDetalizationLevel <= Detail::DEBUG)
	{
		if (_detail <= Detail::DEBUG)
		{
			_sink->push(Detail::DEBUG, _name, message);
		}
	}
}

void Log::debug(const char* fmt, ...)
{
	if constexpr (minimalDetalizationLevel <= Detail::DEBUG)
	{
		if (_detail <= Detail::DEBUG)
		{
			va_list ap;
			va_start(ap, fmt);
			_sink->push(Detail::DEBUG, _name, fmt, ap);
			va_end(ap);
		}
	}
}

void Log::info(const std::string& message)
{
	if constexpr (minimalDetalizationLevel <= Detail::INFO)
	{
		if (_detail <= Detail::INFO)
		{
			_sink->push(Detail::INFO, _name, message);
		}
	}
}

void Log::info(const char* fmt, ...)
{
	if constexpr (minimalDetalizationLevel <= Detail::INFO)
	{
		if (_detail <= Detail::INFO)
		{
			va_list ap;
			va_start(ap, fmt);
			_sink->push(Detail::INFO, _name, fmt, ap);
			va_end(ap);
		}
	}
}

void Log::warn(const std::string& message)
{
	if (_detail <= Detail::WARN)
	{
		_sink->push(Detail::WARN, _name, message);
	}
}

void Log::warn(const char* fmt, ...)
{
	if (_detail <= Detail::WARN)
	{
		va_list ap;
		va_start(ap, fmt);
		_sink->push(Detail::WARN, _name, fmt, ap);
		va_end(ap);
	}
}

void Log::error(const std::string& message)
{
	if (_detail <= Detail::ERROR)
	{
		_sink->push(Detail::ERROR, _name, message);
	}
}

void Log::error(const char* fmt, ...)
{
	if (_detail <= Detail::ERROR)
	{
		va_list ap;
		va_start(ap, fmt);
		_sink->push(Detail::ERROR, _name, fmt, ap);
		va_end(ap);
	}
}

void Log::critical(const std::string& message)
{
	if (_detail <= Detail::CRITICAL)
	{
		_sink->push(Detail::CRITICAL, _name, message);
	}
}

void Log::critical(const char* fmt, ...)
{
	if (_detail <= Detail::CRITICAL)
	{
		va_list ap;
		va_start(ap, fmt);
		_sink->push(Detail::CRITICAL, _name, fmt, ap);
		va_end(ap);
	}
}

void Log::flush()
{
	_sink->flush();
}
