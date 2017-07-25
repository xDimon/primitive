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

// Log.hpp


#pragma once

#define LOG_TRACE_ON
#define LOG_DEBUG_ON

#include <string>
#include <thread>
#include <P7_Client.h>
#include <P7_Trace.h>

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
	std::unique_ptr<IP7_Trace, IP7_Trace_Deleter> _tracer;
	IP7_Trace::hModule module;

	Detail _detail;

public:
	explicit Log(const std::string& name, Detail detail = Detail::UNDEFINED);
	Log() : Log("") {};

	~Log() = default;

	void setName(const std::string& name);
	void setDetail(Detail detail);

	template<typename... Args>
	void trace(const char* fmt, const Args& ... args)
	{
#if defined(LOG_TRACE_ON)
		if (_tracer)
		{
			_tracer->P7_TRACE(module, fmt, args...);
		}
#endif // defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void debug(const char* fmt, const Args& ... args)
	{
#if defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
		if (_tracer)
		{
			_tracer->P7_DEBUG(module, fmt, args...);
		}
#endif // defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void info(const char* fmt, const Args& ... args)
	{
		if (_tracer)
		{
			_tracer->P7_INFO(module, fmt, args...);
		}
	}

	template<typename... Args>
	void warn(const char* fmt, const Args& ... args)
	{
		if (_tracer)
		{
			_tracer->P7_WARNING(module, fmt, args...);
		}
	}

	template<typename... Args>
	void error(const char* fmt, const Args& ... args)
	{
		if (_tracer)
		{
			_tracer->P7_ERROR(module, fmt, args...);
		}
	}

	template<typename... Args>
	void critical(const char* fmt, const Args& ... args)
	{
		if (_tracer)
		{
			_tracer->P7_CRITICAL(module, fmt, args...);
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
