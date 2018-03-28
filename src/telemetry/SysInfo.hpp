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

// SysInfo.hpp


#pragma once


#include <set>
#include <memory>
#include "Metric.hpp"

class SysInfo final
{
public:
	SysInfo(const SysInfo&) = delete; // Copy-constructor
	SysInfo& operator=(SysInfo const&) = delete; // Copy-assignment
	SysInfo(SysInfo&&) noexcept = delete; // Move-constructor
	SysInfo& operator=(SysInfo&&) noexcept = delete; // Move-assignment

private:
	SysInfo();
	~SysInfo() = default;

public: // temporary public
	static SysInfo &getInstance()
	{
		static SysInfo instance;
		return instance;
	}

	timeval _prevTime;
	timeval _prevUTime;
	timeval _prevSTime;

	std::shared_ptr<Metric> _cpuUsageOnPercent;
	std::shared_ptr<Metric> _cpuUsageByUserOnTime;
	std::shared_ptr<Metric> _cpuUsageBySystemOnTime;
	std::shared_ptr<Metric> _memoryUsage;
	std::shared_ptr<Metric> _memoryMaxUsage;
	std::shared_ptr<Metric> _pageSoftFaults;
	std::shared_ptr<Metric> _pageHardFaults;
	std::shared_ptr<Metric> _blockInputOperations;
	std::shared_ptr<Metric> _blockOutputOperations;
	std::shared_ptr<Metric> _voluntaryContextSwitches;
	std::shared_ptr<Metric> _involuntaryContextSwitches;
	bool _run;

public:
	static void start();
	static void collect();
};
