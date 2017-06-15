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

// Server.hpp


#pragma once

#include <memory>
#include <map>
#include "../configs/Config.hpp"
#include "../transport/ServerTransport.hpp"
#include "../storage/DbConnectionPool.hpp"
#include "../services/Service.hpp"
#include <map>

class Server final
{
private:
	static Server* _instance;

	Log _log;

	std::recursive_mutex _mutex;

	std::shared_ptr<Config> _configs;

	std::map<const std::string, std::shared_ptr<ServerTransport>> _transports;
	std::map<const std::string, std::shared_ptr<Service>> _services;

public:
	Server(std::shared_ptr<Config> &configs);
	virtual ~Server();

	static Server& getInstance()
	{
		if (!_instance)
		{
			throw std::runtime_error("Server isn't instantiate yet");
		}
		return *_instance;
	}

	static std::string httpName()
	{
		return "Primitive";
	}

	bool addTransport(const std::string& name, std::shared_ptr<ServerTransport>& transport);

	std::shared_ptr<ServerTransport> getTransport(const std::string& name);

	void enableTransport(const std::string& name);

	void disableTransport(const std::string& name);

	void removeTransport(const std::string& name);

	bool addService(const std::string& name, std::shared_ptr<Service>& service);

	void removeService(const std::string& name);

	void start();
	void stop();

	void wait();
};
