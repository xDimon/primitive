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
private:
	std::string _name;

	std::unique_ptr<IP7_Trace, IP7_Trace_Deleter> _logTrace;
	IP7_Trace::hModule _logModule;

public:
	Log();

	explicit Log(std::string name);

	virtual ~Log();

	virtual Log &log()
	{
		return *this;
	}

	virtual const std::string& name() const
	{
		return _name;
	}

	template<typename... Args>
	void trace(const char *fmt, const Args &... args)
	{
#if defined(LOG_TRACE_ON)
		_logTrace->P7_TRACE(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif // defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void debug(const char *fmt, const Args &... args)
	{
#if defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
		_logTrace->P7_DEBUG(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif // defined(LOG_DEBUG_ON) || defined(LOG_TRACE_ON)
	}

	template<typename... Args>
	void info(const char *fmt, const Args &... args)
	{
		_logTrace->P7_INFO(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	template<typename... Args>
	void warn(const char *fmt, const Args &... args)
	{
		_logTrace->P7_WARNING(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	template<typename... Args>
	void error(const char *fmt, const Args &... args)
	{
		_logTrace->P7_ERROR(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	template<typename... Args>
	void critical(const char *fmt, const Args &... args)
	{
		_logTrace->P7_CRITICAL(_logModule, fmt, args...);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
};
