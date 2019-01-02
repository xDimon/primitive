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

// main.cpp


#include <iostream>
#include "../src/thread/ThreadPool.hpp"
#include "../src/configs/Options.hpp"
#include "../src/configs/Config.hpp"
#include "../src/server/Server.hpp"
#include "../src/utils/Daemon.hpp"
#include "../src/log/LoggerManager.hpp"

int main(int argc, char *argv[])
{
	Daemon::SetProcessName();
	Daemon::StartManageSignals();

	std::shared_ptr<Options> options;
	try
	{
		options = std::make_shared<Options>(argc, argv);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Fail get opptions ← " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	std::shared_ptr<Config> configs;
	try
	{
		configs = std::make_shared<Config>(options);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Fail get configuration ← " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		LoggerManager::init(configs);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Fail logging initialize ← " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	Log log("Main");
	log.info("Start daemon");

	std::shared_ptr<Server> server;
	try
	{
		server = std::make_shared<Server>(configs);
	}
	catch (const std::exception& exception)
	{
		log.error("Fail init server ← %s", exception.what());
		std::cerr << "Fail init server ← " << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

//	Daemon::SetDaemonMode();

	server->start();

	Daemon::doAtShutdown([server](){
		server->stop();
	});

	server->wait();

	log.info("Stop daemon");

	LoggerManager::finalFlush();

	return EXIT_SUCCESS;
}
