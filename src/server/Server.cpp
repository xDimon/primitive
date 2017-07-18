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

Server* Server::_instance = nullptr;

Server::Server(const std::shared_ptr<Config>& configs)
: _log("Server")
, _configs(configs)
{
	if (_instance)
	{
		throw std::runtime_error("Server already instantiated");
	}

	_instance = this;

	_log.info("Server instantiate");

	try
	{
		const auto& settings = _configs->getRoot()["applications"];

		for (auto i = 0; i < settings.getLength(); i++)
		{
			const auto& setting = settings[i];
//		for (const auto& setting : settings)
//		{
			Applications::add(setting);
		}
	}
	catch (const std::exception& exception)
	{
		_log.error("Can't init one of application ← %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["databases"];

		for (auto i = 0; i < settings.getLength(); i++)
		{
			const auto& setting = settings[i];
//		for (const auto& setting : settings)
//		{
			DbManager::openPool(setting);
		}
	}
	catch (const std::runtime_error& exception)
	{
		_log.error("Can't init one of database connection pool ← %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["transports"];

		for (auto i = 0; i < settings.getLength(); i++)
		{
			const auto& setting = settings[i];
//		for (const auto& setting : settings)
//		{
			Transports::add(setting);
		}
	}
	catch (const std::runtime_error& exception)
	{
		_log.error("Can't init one of transport ← %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["services"];

		for (auto i = 0; i < settings.getLength(); i++)
		{
			const auto& setting = settings[i];
//		for (const auto& setting : settings)
//		{
			auto service = ServiceFactory::create(setting);

			if (!addService(service->name(), service))
			{
				throw std::runtime_error(std::string("Already exists service with the same name ('") + service->name() + "')");
			}
		}
	}
	catch (const libconfig::SettingNotFoundException& exception)
	{
		_log.error("Services' config not found");
		throw std::runtime_error("Services' config not found");
	}
	catch (const std::runtime_error& exception)
	{
		_log.error("Can't init one of service ← %s", exception.what());
	}

	ThreadPool::enqueue(
		std::make_shared<Task::Func>(
			ConnectionManager::dispatch
		)
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

void Server::start()
{
	_log.info("Server start");

	ThreadPool::hold();
	ThreadPool::setThreadNum(3);

	Transports::enableAll();
}

void Server::stop()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	while (!getInstance()._services.empty())
	{
		auto i = getInstance()._services.begin();
		auto& service = i->second;
		service->deactivate();
		getInstance()._services.erase(i);
	}

	Transports::disableAll();

	ThreadPool::setThreadNum(0);

	ThreadPool::unhold();

	_log.info("Server stop");
}

bool Server::addService(const std::string& name, const std::shared_ptr<Service>& service)
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	auto i = _services.find(name);
	if (i != _services.end())
	{
		return false;
	}
	service->activate();
	_services.emplace(name, service->ptr());

	return true;
}

void Server::removeService(const std::string& name)
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	auto i = _services.find(name);
	if (i == _services.end())
	{
		return;
	}
	auto& service = i->second;
	service->deactivate();
	_services.erase(i);
}

std::shared_ptr<Service> Server::getService(const std::string& name)
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	auto i = _services.find(name);
	if (i == _services.end())
	{
		return nullptr;
	}
	return i->second;
}
