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
// File created on: 2017.08.14

// TelemetryManager.hpp


#pragma once

#include <deque>
#include <memory>
#include <map>
#include <mutex>
#include "Metric.hpp"

class TelemetryManager final
{
public:
	TelemetryManager(TelemetryManager const&) = delete;
	void operator= (TelemetryManager const&) = delete;
	TelemetryManager(TelemetryManager&&) noexcept = delete;
	TelemetryManager& operator=(TelemetryManager&&) noexcept = delete;

private:
	TelemetryManager() = default;
	~TelemetryManager() = default;

	static TelemetryManager &getInstance()
	{
		static TelemetryManager instance;
		return instance;
	}

	std::mutex _mutex;

	std::map<std::string, std::shared_ptr<Metric>> _metrics;

public:
	static std::shared_ptr<Metric> metric(
		const std::string& name,
		std::chrono::steady_clock::duration ttl,
		std::chrono::steady_clock::duration frame = std::chrono::milliseconds(100),
		Metric::type alertMin = std::numeric_limits<Metric::type>::min(),
		Metric::type alertMax = std::numeric_limits<Metric::type>::max(),
		Metric::type min = std::numeric_limits<Metric::type>::min(),
		Metric::type max = std::numeric_limits<Metric::type>::max()
	);
	static std::shared_ptr<Metric> metric(
		const std::string& name,
		size_t maxCount,
		Metric::type alertMin = std::numeric_limits<Metric::type>::min(),
		Metric::type alertMax = std::numeric_limits<Metric::type>::max(),
		Metric::type min = std::numeric_limits<Metric::type>::min(),
		Metric::type max = std::numeric_limits<Metric::type>::max()
	);

	static const std::map<std::string, std::shared_ptr<Metric>>& metrics();
};
