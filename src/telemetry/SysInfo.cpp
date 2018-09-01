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
// File created on: 2017.08.17

// SysInfo.cpp


#include "SysInfo.hpp"
#include "TelemetryManager.hpp"
#include "../thread/ThreadPool.hpp"
#include "../utils/Daemon.hpp"
#include "../thread/TaskManager.hpp"
#include "../utils/Time.hpp"

#include <sys/resource.h>
#include <sys/time.h>
#include <cstring>

SysInfo::SysInfo()
: _log("Sysinfo")
{
	rusage ru{};
	getrusage(RUSAGE_SELF,&ru);

	gettimeofday(&_prevTime, nullptr);
	_prevUTime = ru.ru_utime;
	_prevSTime = ru.ru_stime;
	_run = false;
}

void SysInfo::start()
{
	auto& instance = getInstance();

	instance._cpuUsageOnPercent				= TelemetryManager::metric("core/cpu/load", std::chrono::seconds(300));
	instance._cpuUsageByUserOnTime			= TelemetryManager::metric("core/cpu/user", std::chrono::seconds(300));
	instance._cpuUsageBySystemOnTime		= TelemetryManager::metric("core/cpu/system", std::chrono::seconds(300));
	instance._memoryUsage					= TelemetryManager::metric("core/mem/phys_usage", std::chrono::seconds(300));
	instance._memoryMaxUsage				= TelemetryManager::metric("core/mem/max_usage", 1);
	instance._pageSoftFaults				= TelemetryManager::metric("core/mem/soft_faults", 1);
	instance._pageHardFaults				= TelemetryManager::metric("core/mem/hard_faults", 1);
	instance._blockInputOperations			= TelemetryManager::metric("core/io/block_input", std::chrono::seconds(300));
	instance._blockOutputOperations			= TelemetryManager::metric("core/io/block_output", std::chrono::seconds(300));
	instance._blockInputOperations			= TelemetryManager::metric("core/io/block_input_all", 1);
	instance._blockOutputOperations			= TelemetryManager::metric("core/io/block_output_all", 1);
	instance._voluntaryContextSwitches		= TelemetryManager::metric("core/ipc/vol_cnt_sw", 1);
	instance._involuntaryContextSwitches	= TelemetryManager::metric("core/ipc/invol_cnt_sw", 1);
	instance._run = true;

	TaskManager::enqueue(
		SysInfo::collect,
		std::chrono::seconds(1),
		"Collect system metrics (first time)"
	);
}

void SysInfo::collect()
{
	auto& instance = getInstance();

	rusage ru{};

	timeval nTime;
	timeval dTime;

	timeval uTimeSpent{};
	timeval sTimeSpent{};
	timeval aTimeSpent{};

	gettimeofday(&nTime, nullptr);
	getrusage(RUSAGE_SELF,&ru);

	timersub(&ru.ru_utime, &instance._prevUTime, &uTimeSpent);
	timersub(&ru.ru_stime, &instance._prevSTime, &sTimeSpent);
	timeradd(&uTimeSpent, &sTimeSpent, &aTimeSpent);
	timersub(&nTime, &instance._prevTime, &dTime);

	auto timeUse = (static_cast<double>(aTimeSpent.tv_sec) + static_cast<double>(aTimeSpent.tv_usec)/1000000);

	auto timePass = (static_cast<double>(dTime.tv_sec) + static_cast<double>(dTime.tv_usec)/1000000);

	auto now = std::chrono::steady_clock::now();

	if (timePass > 0)
	{
		instance._cpuUsageOnPercent->addValue(timeUse * 100 / timePass / std::thread::hardware_concurrency(), now);
	}

	instance._cpuUsageByUserOnTime->setValue((static_cast<double>(ru.ru_utime.tv_sec) + static_cast<double>(ru.ru_utime.tv_usec)/1000000), now);
	instance._cpuUsageBySystemOnTime->setValue((static_cast<double>(ru.ru_stime.tv_sec) + static_cast<double>(ru.ru_stime.tv_usec)/1000000), now);
	instance._memoryMaxUsage->setValue(ru.ru_maxrss, now);
	instance._pageSoftFaults->setValue(ru.ru_minflt, now);
	instance._pageHardFaults->setValue(ru.ru_majflt, now);
	instance._blockInputOperations->setValue(ru.ru_inblock, now);
	instance._blockOutputOperations->setValue(ru.ru_oublock, now);
	instance._voluntaryContextSwitches->setValue(ru.ru_nvcsw, now);
	instance._involuntaryContextSwitches->setValue(ru.ru_nivcsw, now);

    FILE* file = fopen("/proc/self/status", "r");
    if (!file)
	{
		instance._log.error("Can't open /proc/self/status: %s", strerror(errno));
	}
	else
	{
		ssize_t rss = -1;

		char line[128];
		while (fgets(line, 128, file) != nullptr)
		{
			if (strncmp(line, "VmRSS:", 6) == 0)
			{
				rss =
					[]
					(char* oneLine)
					{
						// This assumes that a digit will be found and the line ends in " Kb".
						ssize_t i = strlen(oneLine);
						const char* p = oneLine;
						while (*p < '0' || *p > '9') p++;
						oneLine[i - 3] = '\0';
						i = atoi(p);
						return i;
					} (line);
				break;
			}
		}
		fclose(file);

		instance._memoryUsage->setValue(rss, now);
	}

	gettimeofday(&instance._prevTime, nullptr);
	instance._prevUTime = ru.ru_utime;
	instance._prevSTime = ru.ru_stime;

	if (!Daemon::shutingdown())
	{
		TaskManager::enqueue(
			SysInfo::collect,
			std::chrono::seconds(1),
			"Collect system metrics"
		);
	}
}
