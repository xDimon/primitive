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
// File created on: 2017.04.21

// LoggerManager.hpp


#pragma once

#include <mutex>
#include "Log.hpp"
#include "../configs/Config.hpp"
#include "Sink.hpp"

class LoggerManager final
{
public:
	LoggerManager(const LoggerManager&) = delete;
	LoggerManager& operator=(const LoggerManager&) = delete;
	LoggerManager(LoggerManager&& tmp) noexcept = delete;
	LoggerManager& operator=(LoggerManager&& tmp) noexcept = delete;

private:
	LoggerManager() = default;
	~LoggerManager() = default;

	static LoggerManager &getInstance()
	{
		static LoggerManager instance;
		return instance;
	}

	std::mutex _mutex;

	std::map<std::string, std::shared_ptr<Sink>> _sinks;
	std::map<std::string, std::tuple<std::shared_ptr<Sink>, Log::Detail>> _loggers;

public:
	static void init(const std::shared_ptr<Config>& configs);

	static const std::tuple<std::shared_ptr<Sink>, Log::Detail>& getSinkAndLevel(const std::string& loggerName);
	static std::shared_ptr<Sink> getSink(const std::string& sinkName);

	static void finalFlush();

	static void rotate();
};
