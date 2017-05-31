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
#include "../transport/HttpTransport.hpp"
#include "Server.hpp"
#include "../transport/TransportFactory.hpp"
#include "../net/ConnectionManager.hpp"
#include "../storage/mysql/MysqlConnectionPool.hpp"

Server::Server(std::shared_ptr<Config>& configs)
: _log("Server")
, _configs(configs)
{
	_log.info("Server instantiate");

	try
	{
		const auto& settings = _configs->getRoot()["databases"];

		for (const auto& setting : settings)
		{
			_database = std::make_shared<MysqlConnectionPool>(setting);
			_database->touch();
		}
	}
	catch (const std::runtime_error& exception)
	{
		_log.error("Can't create database connection pool: %s", exception.what());
	}

	try
	{
		const auto& settings = _configs->getRoot()["transports"];

		for (const auto& setting : settings)
		{
			auto transport = TransportFactory::create(setting);

			if (!addTransport(transport->name(), transport))
			{
				throw std::runtime_error(std::string("Already exists transport with the same name ('") + transport->name() + "')");
			}
		}
	}
	catch (const std::runtime_error& exception)
	{
		_log.error("Can't add transport: %s", exception.what());
	}

	ThreadPool::enqueue(ConnectionManager::dispatch);
}

Server::~Server()
{
	_log.info("Server shutdown");
}

void Server::wait()
{
	ThreadPool::wait();
}

bool Server::addTransport(const std::string& name, std::shared_ptr<Transport>& transport)
{
	std::lock_guard<std::mutex> guard(_mutex);
	auto i = _transports.find(name);
	if (i != _transports.end())
	{
		return false;
	}
	_log.debug("Transport '%s' added", name.c_str());
	_transports.emplace(name, transport->ptr());
	return true;
}

void Server::enableTransport(const std::string& name)
{
	std::lock_guard<std::mutex> guard(_mutex);
	auto i = _transports.find(name);
	if (i == _transports.end())
	{
		return;
	}
	auto& transport = i->second;
	transport->enable();
}

void Server::disableTransport(const std::string& name)
{
	std::lock_guard<std::mutex> guard(_mutex);
	auto i = _transports.find(name);
	if (i == _transports.end())
	{
		return;
	}
	auto& transport = i->second;
	transport->disable();
}

void Server::removeTransport(const std::string& name)
{
	std::lock_guard<std::mutex> guard(_mutex);
	auto i = _transports.find(name);
	if (i == _transports.end())
	{
		return;
	}
	auto& transport = i->second;
	transport->disable();
	_transports.erase(i);
	_log.debug("Transport '%s' removed", name.c_str());
}

void Server::start()
{
	_log.info("Server start");

	ThreadPool::hold();
	ThreadPool::setThreadNum(3);

	std::lock_guard<std::mutex> guard(_mutex);
	for (auto &&item : _transports)
	{
		item.second->enable();
	}
}

void Server::stop()
{
	std::lock_guard<std::mutex> guard(_mutex);
	for (auto &&item : _transports)
	{
		item.second->disable();
	}

	ThreadPool::unhold();

	_log.info("Server stop");
}
