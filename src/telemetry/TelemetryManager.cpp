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
// File created on: 2017.08.14

// TelemetryManager.cpp


#include "TelemetryManager.hpp"
#include "../thread/ThreadPool.hpp"

std::shared_ptr<Metric> TelemetryManager::metric(
	const std::string& name,
	std::chrono::steady_clock::duration ttl,
	std::chrono::steady_clock::duration frame,
	Metric::type alertMin,
	Metric::type alertMax,
	Metric::type min,
	Metric::type max
)
{
	auto& instance = getInstance();

	std::lock_guard<std::mutex> lockGuard(instance._mutex);

	const auto& i = instance._metrics.find(name);
	if (i != instance._metrics.end())
	{
		return i->second;
	}

	auto metric = std::make_shared<Metric>(
		name,
		ttl,
		frame,
		alertMin,
		alertMax,
		min,
		max
	);

	instance._metrics.emplace(metric->name(), metric);

	return metric;
}

std::shared_ptr<Metric> TelemetryManager::metric(
	const std::string& name,
	size_t maxCount,
	Metric::type alertMin,
	Metric::type alertMax,
	Metric::type min,
	Metric::type max
)
{
	auto& instance = getInstance();

	std::lock_guard<std::mutex> lockGuard(instance._mutex);

	const auto& i = instance._metrics.find(name);
	if (i != instance._metrics.end())
	{
		return i->second;
	}

	auto metric = std::make_shared<Metric>(
		name,
		maxCount,
		alertMin,
		alertMax,
		min,
		max
	);

	instance._metrics.emplace(metric->name(), metric);

	return metric;
}

const std::map<std::string, std::shared_ptr<Metric>>& TelemetryManager::metrics()
{
	auto& instance = getInstance();
	return instance._metrics;
}
