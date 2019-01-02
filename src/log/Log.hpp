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

// Log.hpp


#pragma once

#define LOG_TRACE_ON
#define LOG_DEBUG_ON

#include <string>
#include <thread>

class Sink;

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
	std::string _name;
	std::shared_ptr<Sink> _sink;
	Detail _detail;

public:
	Log() = delete;
	Log(const Log&) = delete;
	Log& operator=(Log const&) = delete;
	Log(Log&& tmp) noexcept = delete;
	Log& operator=(Log&& tmp) noexcept = delete;

	explicit Log(const std::string& name, Detail detail = Detail::UNDEFINED, const std::string& sink = "");

	~Log() = default;

	void setName(const std::string& name);

	Detail detail() const
	{
		return _detail;
	}
	void setDetail(Detail detail);

	void trace(const std::string& message);
	void trace(const char* fmt, ...);

	void debug(const std::string& message);
	void debug(const char* fmt, ...);

	void info(const std::string& message);
	void info(const char* fmt, ...);

	void warn(const std::string& message);
	void warn(const char* fmt, ...);

	void error(const std::string& message);
	void error(const char* fmt, ...);

	void critical(const std::string& message);
	void critical(const char* fmt, ...);

	void flush();
};
