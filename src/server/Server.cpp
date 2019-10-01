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
// File created on: 2017.02.25

// Server.cpp


#include <utils/Daemon.hpp>
#include <log/LoggerManager.hpp>
#include "Server.hpp"
#include <serialization/SArr.hpp>
#include <extra/Applications.hpp>
#include <storage/DbManager.hpp>
#include <transport/Transports.hpp>
#include <services/Services.hpp>
#include <telemetry/SysInfo.hpp>
#include <net/ConnectionManager.hpp>
#include <thread/TaskManager.hpp>
#include <thread/ThreadPool.hpp>


Server* Server::_instance = nullptr;

Server::Server(const SObj& configs)
: _log("Server")
, _workerCount(0)
{
	if (_instance != nullptr)
	{
		throw std::runtime_error("Server already instantiated");
	}

	_instance = this;

	_log.info("Server instantiate");

	try
	{
		const auto& settings = configs.getAs<SObj>("core");

		if (settings.has("timeZone"))
		{
			std::string timeZone;
			settings.lookup("timeZone", timeZone);
			if (!timeZone.empty())
			{
				setenv("TZ", timeZone.c_str(), 1);
				tzset();
			}
		}

		if (settings.has("processName"))
		{
			std::string processName;
			settings.lookup("processName", processName);
			if (!processName.empty())
			{
				Daemon::SetProcessName(processName);
			}
		}

		if (settings.hasOf<SStr>("workers"))
		{
			if (settings.getAs<SStr>("workers") == "auto")
			{
				_workerCount = std::max<size_t>(2, std::thread::hardware_concurrency());
			}
		}
		if (_workerCount == 0)
		{
			_workerCount = settings.getAs<SInt>("workers");
			if (_workerCount < 2)
			{
				throw std::runtime_error("Count of workers too few. Programm won't be work correctly");
			}
		}
	}
	catch (const std::exception& exception)
	{
		_log.critical("Can't configure of core ← %s", exception.what());
		LoggerManager::finalFlush();
		exit(EXIT_FAILURE);
	}

	if (configs.has("applications"))
	{
		try
		{
			for (const auto& setting : configs.getAs<SArr>("applications"))
			{
				Applications::add(setting.as<SObj>());
			}
		}
		catch (const std::exception& exception)
		{
			_log.error("Can't init one of application ← %s", exception.what());
		}
	}

	if (configs.has("databases"))
	{
		for (const auto& setting : configs.getAs<SArr>("databases"))
		{
			try
			{
				DbManager::openPool(setting.as<SObj>());
			}
			catch (const std::exception& exception)
			{
				_log.warn("Can't init one of database connection pool ← %s", exception.what());
			}
		}
	}

	if (configs.has("transports"))
	{
		for (const auto& [name, setting] : configs.getAs<SObj>("transports"))
		{
			try
			{
				Transports::add(name, setting.as<SObj>());
			}
			catch (const std::exception& exception)
			{
				_log.warn("Can't init transport '%s' ← %s", name.c_str(), exception.what());
			}
		}
	}

	if (configs.has("services"))
	{
		for (const auto& setting : configs.getAs<SArr>("services"))
		{
			try
			{
				Services::add(setting.as<SObj>());
			}
			catch (const std::exception& exception)
			{
				_log.error("Can't init one of service ← %s", exception.what());
			}
		}
	}

	SysInfo::start();

	TaskManager::enqueue(
		ConnectionManager::dispatch,
		"Start ConnectionManager dispatcher"
	);
}

Server::~Server()
{
	_instance = nullptr;
	_log.info("Server shutdown");
}

void Server::wait()
{
	ThreadPool::wait();
}

bool Server::start()
{
	if (!Transports::enableAll())
	{
		_log.info("Can't enable all services");
		return false;
	}

	ThreadPool::setThreadNum(_workerCount);

	_log.info("Server start (pid=%u)", getpid());
	return true;
}

void Server::stop()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	Services::deactivateAll();

	Transports::disableAll();

	_log.info("Server stop");
}
