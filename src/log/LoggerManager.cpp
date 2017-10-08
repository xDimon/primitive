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
// File created on: 2017.04.21

// LoggerManager.cpp


#include <stdexcept>
#include <thread>
#include "LoggerManager.hpp"

void LoggerManager::init(const std::shared_ptr<Config> &configs)
{
	auto& lm = getInstance();

	const Setting* loggingSetting;
	try
	{
		loggingSetting = &configs->getRoot()["logs"];
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		loggingSetting = nullptr;
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string() + "Can't get configuration ← " + exception.what());
	}

	if (loggingSetting != nullptr)
	{
		std::lock_guard<std::mutex> lockGuard(lm._mutex);

		try
		{
			const auto& settings = (*loggingSetting)["sinks"];

			for (auto i = 0; i < settings.getLength(); i++)
			{
				const auto& setting = settings[i];
//			for (const auto& setting : settings)
//			{
				auto sink = std::make_shared<Sink>(setting);

				if (lm._sinks.find(sink->name()) != lm._sinks.end())
				{
					throw std::runtime_error("Duplicate sink with name '" + sink->name() + "'");
				}

				lm._sinks.emplace(sink->name(), sink);
			}
		}
		catch (const libconfig::SettingNotFoundException& exception)
		{
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error(std::string() + "Can't make sink ← " + exception.what());
		}

		try
		{
			const auto& settings = (*loggingSetting)["loggers"];

			for (auto i = 0; i < settings.getLength(); i++)
			{
				const auto& setting = settings[i];
//			for (const auto& setting : settings)
//			{
				std::string name;
				if (!setting.lookupValue("name", name))
				{
					throw std::runtime_error("Not found name for one of loggers");
				}
				if (lm._loggers.find(name) != lm._loggers.end())
				{
					throw std::runtime_error("Duplicate logger with name '" + name + "'");
				}

				std::string sinkname;
				if (!setting.lookupValue("sink", sinkname))
				{
					throw std::runtime_error("Undefined sink for logger '" + name + "'");
				}
				if (lm._sinks.find(sinkname) == lm._sinks.end())
				{
					throw std::runtime_error("Not found sink '" + sinkname + "' for logger '" + name + "'");
				}
				auto& sink = lm._sinks[sinkname];

				std::string level;
				if (!setting.lookupValue("level", level))
				{
					throw std::runtime_error("Undefined level for logger '" + level + "'");
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
		catch (const libconfig::SettingNotFoundException& exception)
		{
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error(std::string() + "Invalid configuration ← " + exception.what());
		}
	}

	{
		std::lock_guard<std::mutex> lockGuard(lm._mutex);
		if (lm._sinks.empty())
		{
			lm._sinks.emplace("", std::make_shared<Sink>());
		}
		if (lm._loggers.empty())
		{
			lm._loggers.emplace("*", std::make_tuple(lm._sinks[""], Log::Detail::INFO));
		}
	}

	regThread("MainThread");
}

void LoggerManager::regThread(const std::string& threadName)
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	for (auto& i : lm._sinks)
	{
		auto& sink = i.second;

		uint32_t tid = 0;//static_cast<uint32_t>(((pthread_self() >> 32) ^ pthread_self()) & 0xFFFFFFFF);
		sink->trace().Register_Thread(threadName.c_str(), tid);
	}
}

void LoggerManager::unregThread()
{
	auto& lm = getInstance();

	std::lock_guard<std::mutex> lockGuard(lm._mutex);

	for (auto& i : lm._sinks)
	{
		auto& sink = i.second;

		uint32_t tid = 0;//static_cast<uint32_t>(((pthread_self() >> 32) ^ pthread_self()) & 0xFFFFFFFF);
		sink->trace().Unregister_Thread(tid);
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
