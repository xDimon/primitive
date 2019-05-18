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
// File created on: 2017.05.21

// Daemon.hpp


#pragma once


#include <functional>
#include <mutex>
#include <deque>
#include <csignal>

class Daemon final
{
public:
	Daemon(const Daemon&) = delete;
	Daemon& operator=(const Daemon&) = delete;
	Daemon(Daemon&&) noexcept = delete;
	Daemon& operator=(Daemon&&) noexcept = delete;

private:
	Daemon();
	~Daemon();

	static Daemon &getInstance()
	{
		static Daemon instance;
		return instance;
	}

	std::mutex _mutex;

	bool _shutingdown;

	std::deque<std::function<void ()>> _handlers;

public:
	static const std::string& ExePath();

	static void SetProcessName(const std::string& processName = "");

	static void SignalsHandler(int sig, siginfo_t* info, void* context);

	static void StartManageSignals();

	static void SetDaemonMode();

	static void doAtShutdown(std::function<void ()> handler);
	static void shutdown();
	static bool shutingdown()
	{
		return getInstance()._shutingdown;
	}
};
