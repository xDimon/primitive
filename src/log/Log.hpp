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

//#define LOG_TRACE_ON
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

class Log
{
public:
	std::unique_ptr<IP7_Trace, IP7_Trace_Deleter> trace;
	IP7_Trace::hModule module;

	Log();

	explicit Log(std::string name);

	virtual ~Log();

	virtual Log &log()
	{
		return *this;
	}

	template<typename... Args>
	void trace_(const char* fmt, const Args& ... args)
	{
#if defined(LOG_TRACE_ON)
		trace->P7_TRACE(module, fmt, args...);
#endif // defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void debug(const char *fmt, const Args &... args)
	{
#if defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
		trace->P7_DEBUG(module, fmt, args...);
#endif // defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void info(const char *fmt, const Args &... args)
	{
		trace->P7_INFO(module, fmt, args...);
	}

	template<typename... Args>
	void warn(const char *fmt, const Args &... args)
	{
		trace->P7_WARNING(module, fmt, args...);
	}

	template<typename... Args>
	void error(const char *fmt, const Args &... args)
	{
		trace->P7_ERROR(module, fmt, args...);
	}

	template<typename... Args>
	void critical(const char *fmt, const Args &... args)
	{
		trace->P7_CRITICAL(module, fmt, args...);
	}
};
