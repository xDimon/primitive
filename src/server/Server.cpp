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
// File created on: 2017.02.25

// Server.cpp


#include "../net/SslAcceptor.hpp"
#include "../thread/ThreadPool.hpp"
#include "Server.hpp"
#include "../net/ConnectionManager.hpp"
#include "../storage/DbManager.hpp"
#include "../extra/Applications.hpp"
#include "../transport/Transports.hpp"
#include "../telemetry/SysInfo.hpp"
#include "../services/Services.hpp"
#include "../utils/Daemon.hpp"
#include "../thread/TaskManager.hpp"

Server* Server::_instance = nullptr;

Server::Server(const std::shared_ptr<Config>& configs)
: _log("Server")
, _workerCount(0)
, _configs(configs)
{
	if (_instance != nullptr)
	{
		throw std::runtime_error("Server already instantiated");
	}

	_instance = this;

	_log.info("Server instantiate");

	try
	{
		const auto& settings = _configs->getRoot()["core"];

		std::string processName;
		if (settings.lookupValue("processName", processName) && !processName.empty())
		{
			Daemon::SetProcessName(processName);
		}
		if (!settings.lookupValue("workers", _workerCount))
		{
			_workerCount = std::thread::hardware_concurrency();
		}
		if (_workerCount < 2)
		{
			throw std::runtime_error("Count of workers too few. Programm won't be work correctly");
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.warn("Core config not found");
	}
	catch (const std::exception& exception)
	{
		_log.error("Can't configure of core ← %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["applications"];

		for (const auto& setting : settings)
		{
			Applications::add(setting);
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.warn("Applications' config not found");
	}
	catch (const std::exception& exception)
	{
		_log.error("Can't init one of application ← %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["databases"];

		for (const auto& setting : settings)
		{
			try
			{
				DbManager::openPool(setting);
			}
			catch (const std::exception& exception)
			{
				_log.warn("Can't init one of database connection pool ← %s", exception.what());
			}
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.warn("Databases' config not found");
	}

	try
	{
		const auto& settings = _configs->getRoot()["transports"];

		for (const auto& setting : settings)
		{
			try
			{
				Transports::add(setting);
			}
			catch (const std::exception& exception)
			{
				_log.warn("Can't init one of transport ← %s", exception.what());
			}
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.warn("Transports' config not found");
	}

	try
	{
		const auto& settings = _configs->getRoot()["services"];

		for (const auto& setting : settings)
		{
			try
			{
				Services::add(setting);
			}
			catch (const std::exception& exception)
			{
				_log.error("Can't init one of service ← %s", exception.what());
			}
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.error("Services' config not found");
	}

	SysInfo::start();

	TaskManager::enqueue(
		ConnectionManager::dispatch
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
