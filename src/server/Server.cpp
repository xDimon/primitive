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
#include "../transport/PacketTransport.hpp"
#include "../transport/WsTransport.hpp"
#include "../utils/SslHelper.hpp"
#include "Server.hpp"
#include "../net/ConnectionManager.hpp"

Server::Server(Configs::Ptr &configs)
: Log("Server")
, _configs(configs)
{
	addTransport("0.0.0.0:8000", new HttpTransport(TcpAcceptor::create, "0.0.0.0", 8000));
	addTransport("0.0.0.0:8001", new PacketTransport(TcpAcceptor::create, "0.0.0.0", 8001));
	addTransport("0.0.0.0:8002", new WsTransport(TcpAcceptor::create, "0.0.0.0", 8002));
	addTransport("0.0.0.0:4430", new HttpTransport(SslAcceptor::create, "0.0.0.0", 4430, SslHelper::context()));
	addTransport("0.0.0.0:4301", new PacketTransport(SslAcceptor::create, "0.0.0.0", 4431, SslHelper::context()));
	addTransport("0.0.0.0:4432", new WsTransport(SslAcceptor::create, "0.0.0.0", 4432, SslHelper::context()));

	ThreadPool::enqueue([](){
		ConnectionManager::dispatch();
	});
}

Server::~Server()
{
}

void Server::wait()
{
	ThreadPool::wait();
}

bool Server::addTransport(const std::string name, Transport *transport)
{
	std::lock_guard<std::mutex> guard(_mutex);
	auto i = _transports.find(name);
	if (i != _transports.end())
	{
		return false;
	}
	_transports.emplace(name, Transport::Ptr(transport));
	return true;
}

void Server::enableTransport(const std::string name)
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

void Server::disableTransport(const std::string name)
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

void Server::removeTransport(const std::string name)
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
}

void Server::start()
{
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
}
