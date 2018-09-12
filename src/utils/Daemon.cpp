// Copyright © 2017-2018 Dmitriy Khaustov
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
#include "../thread/ThreadPool.hpp"
#include "../log/LoggerManager.hpp"

#include <cxxabi.h>
#include <climits>
#include <set>
#include <execinfo.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <sys/resource.h>

Daemon::Daemon()
: _shutingdown(false)
{
	// Включаем генерацию core-файла
	rlimit corelimit = { RLIM_INFINITY, RLIM_INFINITY };
	setrlimit(RLIMIT_CORE, &corelimit);

	atexit(Daemon::shutdown);
};

Daemon::~Daemon()
{
	shutdown();
};

const std::string& Daemon::ExePath()
{
	static std::string result;
	if (result.empty())
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

void Daemon::SetProcessName(const std::string& processName)
{
	if (processName.empty())
	{
	// Назначаем имя процесса
#ifdef PROJECT_NAME
  #define HELPER4QUOTE(N) #N
	prctl(PR_SET_NAME, HELPER4QUOTE(PROJECT_NAME));
  #undef HELPER4QUOTE
#else
		prctl(PR_SET_NAME, "primitive");
#endif
	}
	else
	{
		prctl(PR_SET_NAME, processName.c_str());
	}
}

void Daemon::SetDaemonMode()
{
	if (daemon(0, 0) == -1)
	{
		throw std::runtime_error(std::string("Can't to become daemon ← ") + strerror(errno));
	}
}

void Daemon::StartManageSignals()
{
	stack_t ss{};
	ss.ss_size = SIGSTKSZ;//PTHREAD_STACK_MIN;
	ss.ss_sp = mmap(
		nullptr, ss.ss_size,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANON, -1, 0
	);

	sigaltstack(&ss, nullptr);

	// Игнорируемые сигналы
	std::set<int> ignoredSignals = {
		SIGHUP,
		SIGPIPE
	};

	struct sigaction act{};
	memset(&act, 0, sizeof(act));

	act.sa_flags = SA_SIGINFO; // обработчик требует 3 аргумента
	act.sa_sigaction = Daemon::SignalsHandler; // обработчик

	for (int n = 1; n < _NSIG; n++)
	{
		sigset_t mask{};
		sigemptyset(&mask);
		sigaddset(&mask, n);
		if (ignoredSignals.find(n) != ignoredSignals.end())
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

void Daemon::SignalsHandler(int sig, siginfo_t* info, void* context)
{
	static volatile bool fatalError = false;
	bool needBacktrace;
	bool needReraise = false;
	bool needAbort = false;

	{
		Log log("Signals");

		switch (sig)
		{
			case SIGHUP:
				log.info("Daemon begin to reload...");
				log.flush();
				Daemon::shutdown();
				// TODO Реализовать перезагрузку
				return;

			case SIGQUIT:
				log.info("Daemon will be stopped now (quit)");
				log.flush();
				Daemon::shutdown();
				return;

			case SIGTERM:
				log.info("Daemon will be stopped now (terminate)");
				log.flush();
				Daemon::shutdown();
				return;

			case SIGINT:
				log.info("Daemon will be stopped now (interrupt)");
				log.flush();
				Daemon::shutdown();
				return;

			case SIGSEGV:
				if (fatalError)
				{
					LoggerManager::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.critical("Sigmentation fail. Terminate daemon!");
				log.flush();
				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGBUS:
				if (fatalError)
				{
					LoggerManager::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.critical("Bus fail. Terminate daemon!");
				log.flush();
				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGABRT:
				if (fatalError)
				{
					LoggerManager::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.warn("Abort. Terminate daemon!");
				log.flush();

				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGFPE:
				if (fatalError)
				{
					LoggerManager::finalFlush();
					raise(sig);
				}
				fatalError = true;
				log.critical("Floating point exception. Terminate daemon!");
				log.flush();

				needBacktrace = true;
				needAbort = true;
				goto actions;

			case SIGUSR1:
				log.info("Log rotation.");
				log.flush();
				LoggerManager::rotate();
				return;

			case SIGUSR2:
				log.info("Received signal USR2. Unload stacks and info");
				log.flush();
				needBacktrace = true;
				goto actions;

			default:
				log.debug("Received signal `%s`", sys_siglist[sig]);
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

		char **strings = backtrace_symbols(bt, n);

		log.info("--- begin backtrace ---");
		for (int i = 0; i < n; i++)
		{
			Dl_info dli{};

			Elf64_Sym *symptr;

			dladdr1(bt[i], &dli, reinterpret_cast<void **>(&symptr), RTLD_DL_SYMENT);

			int status;
			auto symbol = abi::__cxa_demangle(dli.dli_sname, 0, 0, &status);

			if (status == 0)
				log.info("%s(%s)", dli.dli_fname, symbol);
			else if (status == -2)
				log.info("%s(%s)", dli.dli_fname, dli.dli_sname);
			else if (status == -3)
				log.info("%s()", dli.dli_fname);
			else
				log.info("%s(%s)", dli.dli_fname, dli.dli_sname);

//			log.info("[%p+%p]", dli.dli_fbase, dli.dli_saddr);
//
//			log.info("%s\n", strings[i]);
		}

		log.info("---- end backtrace ----");

		free(strings);
	}

	if (needAbort)
	{
		LoggerManager::finalFlush();

		exit(EXIT_FAILURE);
	}

	if (needReraise)
	{
		raise(sig);
	}
}

void Daemon::doAtShutdown(std::function<void()> handler)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	getInstance()._handlers.emplace_front(std::move(handler));
}

void Daemon::shutdown()
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);

	if (!getInstance()._shutingdown)
	{
		getInstance()._shutingdown = true;

		for (const auto& handler : getInstance()._handlers)
		{
			handler();
		}
	}
}
