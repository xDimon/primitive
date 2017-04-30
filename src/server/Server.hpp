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
#include "../transport/Transport.hpp"

class Server: public Log
{
public:
	typedef std::shared_ptr<Server> Ptr;

private:

	std::mutex _mutex;

	Config::Ptr _configs;

	std::map<const std::string, std::shared_ptr<Transport>> _transports;

public:
	Server(Config::Ptr &configs);
	virtual ~Server();

	static std::string httpName()
	{
		return "Primitive";
	}

	bool addTransport(const std::string& name, std::shared_ptr<Transport>& transport);

	void enableTransport(const std::string& name);

	void disableTransport(const std::string& name);

	void removeTransport(const std::string& name);

	void start();
	void stop();

	void wait();
};
