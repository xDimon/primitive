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

// LoggerManager.cpp


#include <stdexcept>
#include <thread>
#include "LoggerManager.hpp"
#include <serialization/SArr.hpp>

void LoggerManager::init(const Setting &configs)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	if (configs.has("sinks"))
	{
		try
		{
			for (const auto& [name, setting] : configs.getAs<SObj>("sinks"))
			{
				auto sink = std::make_shared<Sink>(name, setting.as<SObj>());

				if (lm._sinks.find(sink->name()) != lm._sinks.end())
				{
					throw std::runtime_error("Duplicate sink with name '" + sink->name() + "'");
				}

				lm._sinks.emplace(sink->name(), sink);
			}
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error(std::string() + "Fail configure sinks ← " + exception.what());
		}
	}

	if (configs.has("loggers"))
	{
		try
		{
			for (const auto& [name, setting_] : configs.getAs<SObj>("loggers"))
			{
				const auto& setting = setting_.as<SObj>();

				if (lm._loggers.find(name) != lm._loggers.end())
				{
					throw std::runtime_error("Duplicate logger with name '" + name + "'");
				}

				std::string sinkname;
				try
				{
					sinkname = setting.getAs<SStr>("sink");
				}
				catch (const std::exception& exception)
				{
					throw std::runtime_error("Undefined sink for logger '" + name + "'");
				}
				auto i = lm._sinks.find(sinkname);
				if (i == lm._sinks.end())
				{
					throw std::runtime_error("Not found sink '" + sinkname + "' for logger '" + name + "'");
				}
				auto sink = i->second;

				std::string level;
				try
				{
					level = setting.getAs<SStr>("level");
				}
				catch (const std::exception& exception)
				{
					throw std::runtime_error("Undefined level for logger '" + name + "'");
				}
				Log::Detail detail = Log::Detail::UNDEFINED;
				if (level == "trace")
				{
					detail = Log::Detail::TRACE;
				}
				else if (level == "debug")
				{
					detail = Log::Detail::DEBUG;
				}
				else if (level == "info")
				{
					detail = Log::Detail::INFO;
				}
				else if (level == "warn")
				{
					detail = Log::Detail::WARN;
				}
				else if (level == "error")
				{
					detail = Log::Detail::ERROR;
				}
				else if (level == "crit")
				{
					detail = Log::Detail::CRITICAL;
				}
				else if (level == "off")
				{
					detail = Log::Detail::OFF;
				}
				else
				{
					throw std::runtime_error("Unknown level ('" + level + "') for logger '" + name + "'");
				}

				lm._loggers.emplace(name, std::make_tuple(sink, detail));
			}
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error(std::string() + "Fail configure loggers ← " + exception.what());
		}
	}
	else if (!lm._sinks.empty())
	{
		throw std::runtime_error(std::string() + "Configured sink(s) without anyone loggers");
	}

	{
//		std::lock_guard<std::mutex> lockGuard(lm._mutex);
		if (lm._loggers.empty())
		{
			lm._loggers.emplace("*", std::make_tuple(std::make_shared<Sink>(), Log::Detail::INFO));
		}
	}
}

const std::tuple<std::shared_ptr<Sink>, Log::Detail>& LoggerManager::getSinkAndLevel(const std::string& name)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	auto i = lm._loggers.find(name);

	if (i == lm._loggers.end())
	{
		i = lm._loggers.find("*");
		if (i == lm._loggers.end())
		{
			throw std::runtime_error("Can't get settings for logger");
		}
	}

	return i->second;
}

std::shared_ptr<Sink> LoggerManager::getSink(const std::string& name)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	auto i = lm._sinks.find(name);

	if (i == lm._sinks.end())
	{
		i = lm._sinks.find("");
		if (i == lm._sinks.end())
		{
			throw std::runtime_error("Can't get sink for logger");
		}
	}

	return i->second;
}

void LoggerManager::finalFlush()
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	for (auto& i : lm._sinks)
	{
		i.second->flush();
	}
}

void LoggerManager::rotate()
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	for (auto& i : lm._sinks)
	{
		i.second->rotate();
	}
}
