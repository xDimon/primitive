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
#include "../telemetry/Metric.hpp"

class Server final
{
private:
	static Server* _instance;

	Log _log;

	std::recursive_mutex _mutex;

	uint32_t _workerCount;

	std::shared_ptr<Config> _configs;

public:
	Server(const Server&) = delete;
	void operator=(Server const&) = delete;
	Server(Server&&) = delete;
	Server& operator=(Server&&) = delete;

	explicit Server(const std::shared_ptr<Config> &configs);
	~Server();

	static std::string httpName()
	{
		#if defined(PROJECT_NAME)
		 #define HELPER4QUOTE(N) #N
			return "primitive for " HELPER4QUOTE(PROJECT_NAME);
		 #undef HELPER4QUOTE
		#else
			return "primitive";
		#endif
	}

	void start();
	void stop();

	void wait();
};
