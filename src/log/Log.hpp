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
// File created on: 2017.03.11

// Log.hpp


#pragma once

#define LOG_TRACE_ON
#define LOG_DEBUG_ON

#include <string>
#include <thread>
#include <P7_Client.h>
#include <P7_Trace.h>
#include "Sink.hpp"

struct IP7_Trace_Deleter
{
	void operator()(IP7_Trace* logTrace)
	{
		logTrace->Release();
	}
};

class Log final
{
public:
	enum class Detail
	{
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		CRITICAL,
		OFF,
		UNDEFINED
	};

private:
	IP7_Trace::hModule module;

	std::shared_ptr<Sink> _sink;
	Detail _detail;

public:
	Log() = delete;
	Log(const Log&) = delete;
	void operator=(Log const&) = delete;
	Log(Log&& tmp) = delete;
	Log& operator=(Log&& tmp) = delete;

	explicit Log(const std::string& name, Detail detail = Detail::UNDEFINED, const std::string& sink = "");

	~Log() = default;

	void setName(const std::string& name);

	Detail detail() const
	{
		return _detail;
	}
	void setDetail(Detail detail);

	template<typename... Args>
	void trace(const char* fmt, const Args& ... args)
	{
#if defined(LOG_TRACE_ON)
		if (_detail == Detail::TRACE)
		{
			_sink->trace().P7_TRACE(module, fmt, args...);
		}
#endif // defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void debug(const char* fmt, const Args& ... args)
	{
#if defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
		if (_detail <= Detail::DEBUG)
		{
			_sink->trace().P7_DEBUG(module, fmt, args...);
		}
#endif // defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void info(const char* fmt, const Args& ... args)
	{
		if (_detail <= Detail::INFO)
		{
			_sink->trace().P7_INFO(module, fmt, args...);
		}
	}

	template<typename... Args>
	void warn(const char* fmt, const Args& ... args)
	{
		if (_detail <= Detail::WARN)
		{
			_sink->trace().P7_WARNING(module, fmt, args...);
		}
	}

	template<typename... Args>
	void error(const char* fmt, const Args& ... args)
	{
		if (_detail <= Detail::ERROR)
		{
			_sink->trace().P7_ERROR(module, fmt, args...);
		}
	}

	template<typename... Args>
	void critical(const char* fmt, const Args& ... args)
	{
		if (_detail <= Detail::CRITICAL)
		{
			_sink->trace().P7_CRITICAL(module, fmt, args...);
		}
	}

	void flush()
	{
	}

	static void finalFlush()
	{
		P7_Exceptional_Flush();
	}
};
