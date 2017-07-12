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
// File created on: 2017.05.21

// Daemon.cpp


#include "Daemon.hpp"
#include "../log/Log.hpp"
#include "../thread/ThreadPool.hpp"
#include "ShutdownManager.hpp"

#include <unistd.h>
#include <climits>
#include <sstream>
#include <csignal>
#include <cstring>
#include <set>
#include <execinfo.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <cxxabi.h>

static void SignalsHandler(int sig, siginfo_t* info, void* context);

const std::string& ExePath()
{
	static std::string result;
	if (!result.length())
	{
		std::vector<char> path;
		path.resize(PATH_MAX);
		ssize_t len = readlink("/proc/self/exe", &path[0], path.size());
		if (len > 0)
		{
			path.resize(static_cast<size_t>(len));
			result.assign(path.begin(), path.end());
		}
	}
	return result;
}

void SetProcessName()
{
	// Назначаем имя процесса
#define PRIMITIVE_DEF_NAME primitive
#define HELPER4QUOTE(N) #N
#if defined(PROJECT_NAME) && PROJECT_NAME!=PRIMITIVE_DEF_NAME
	prctl(PR_SET_NAME, HELPER4QUOTE(PROJECT_NAME));
#else
	prctl(PR_SET_NAME, HELPER4QUOTE(PRIMITIVE_DEF_NAME));
#endif
#undef HELPER4QUOTE
#undef PRIMITIVE_DEF_NAME
}

void SetDaemonMode()
{
	if (daemon(0, 0) == -1)
	{
		throw std::runtime_error(std::string("Can't to become daemon ← ") + strerror(errno));
	}
}

void StartManageSignals()
{
	// Игнорируемые сигналы
	std::set<int> ignoredSignals = {
		SIGHUP,
		SIGPIPE
	};

	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_flags = SA_SIGINFO; // обработчик требует 3 аргумента
	act.sa_sigaction = SignalsHandler; // обработчик

	for (int n = 1; n < _NSIG; n++)
	{
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, n);
		if (ignoredSignals.count(n))
		{
			sigprocmask(SIG_BLOCK, &mask, nullptr);
		}
		else
		{
			sigaction(n, &act, nullptr);
			sigprocmask(SIG_UNBLOCK, &mask, nullptr);
		}
	}
}

void SignalsHandler(int sig, siginfo_t* info, void* context)
{
	static volatile bool fatalError = false;
	bool needBacktrace = false;
	bool needReraise = false;
	bool needAbort = false;

	{
		Log log("Signals");

		switch (sig)
		{
			case SIGHUP:
				log.info("Daemon begin to reload...");
				log.flush();
				ShutdownManager::shutdown();
				// TODO Реализовать перезагрузку
				return;

			case SIGQUIT:
				log.info("Daemon will be stopped now (quit)");
				log.flush();
				ShutdownManager::shutdown();
				return;

			case SIGTERM:
				log.info("Daemon will be stopped now (terminate)");
				log.flush();
				ShutdownManager::shutdown();
				return;

			case SIGINT:
				log.info("Daemon will be stopped now (interrupt)");
				log.flush();
				ShutdownManager::shutdown();
				return;

			case SIGSEGV:
				if (fatalError)
				{
					Log::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.info("Sigmentation fail. Terminate daemon!");
				log.flush();
				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGBUS:
				if (fatalError)
				{
					Log::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.info("Bus fail. Terminate daemon!");
				log.flush();
				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGABRT:
				if (fatalError)
				{
					Log::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.info("Abort. Terminate daemon!");
				log.flush();

				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGFPE:
				if (fatalError)
				{
					Log::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.info("Floating point exception. Terminate daemon!");
				log.flush();

				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGUSR1:
				log.info("Received signal USR1. Reconfigure");
				log.flush();
				// TODO реализовать переконфигурацию
				return;

			case SIGUSR2:
				log.info("Received signal USR2. Unload stacks and info");
				log.flush();
				needBacktrace = true;
				goto actions;

			default:
				log.info("Received signal `%s`", sys_siglist[sig]);
				log.flush();
		}
	}
	return;

	actions:

	// Выгрузка стека
	if (needBacktrace)
	{
		void *bt[128];
		int n = backtrace(bt, 128);


		Log log("Backtrace");

		log.debug("--- begin backtrace ---");

		char **strings = backtrace_symbols(bt, n);
//		for (int i = 0; i < n; i++)
//		{
//			log.debug(strings[i]);
//		}

		for (int i = 0; i < n; i++)
		{
			Dl_info dli;

			dladdr(bt[0], &dli);

			int status;
			auto symbol = abi::__cxa_demangle(dli.dli_sname, 0, 0, &status);

			log.info("%s(%s)", dli.dli_fname, symbol);
			log.debug(strings[i]);
		}

		log.debug("---- end backtrace ----");

		free(strings);
	}

	if (needAbort)
	{
		Log::finalFlush();;

		exit(EXIT_FAILURE);
	}

	if (needReraise)
	{
		raise(sig);
	}
}
