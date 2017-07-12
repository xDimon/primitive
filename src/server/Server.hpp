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

	std::map<const std::string, const std::shared_ptr<Service>> _services;

public:
	Server(const std::shared_ptr<Config> &configs);
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
		#define PRIMITIVE_DEF_NAME primitive
		#define HELPER4QUOTE(N) #N
		#if defined(PROJECT_NAME) && (PROJECT_NAME!=PRIMITIVE_DEF_NAME)
			return HELPER4QUOTE(PRIMITIVE_DEF_NAME) " for " HELPER4QUOTE(PROJECT_NAME);
		#else
			return HELPER4QUOTE(PRIMITIVE_DEF_NAME);
		#endif
		#undef HELPER4QUOTE
		#undef PRIMITIVE_DEF_NAME
	}

	bool addService(const std::string& name, const std::shared_ptr<Service>& service);
	std::shared_ptr<Service> getService(const std::string& name);
	void removeService(const std::string& name);

	void start();
	void stop();

	void wait();
};
