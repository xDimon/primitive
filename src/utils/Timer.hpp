// Copyright © 2018 Dmitriy Khaustov
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
// File created on: 2018.10.29

// Timer.hpp


#pragma once

#include <functional>
#include <mutex>
#include "Shareable.hpp"

class Timer final: public Shareable<Timer>
{
private:
	class Helper final: public Shareable<Helper>
	{
	private:
		std::weak_ptr<Timer> _wp;

		void onTime();

	public:
		Helper() = delete; // Default-constructor
		Helper(const Helper&) = delete; // Copy-constructor
		Helper& operator=(Helper const&) = delete; // Copy-assignment
		Helper(Helper&&) noexcept = delete; // Move-constructor
		Helper& operator=(Helper&&) noexcept = delete; // Move-assignment

		explicit Helper(const std::shared_ptr<Timer>& timer);
		~Helper() override = default;

		void start(std::chrono::steady_clock::time_point time);

		void cancel();
	};

	std::recursive_mutex _mutex;

	std::function<void()> _alarmHandler;
	const char *_label;

	std::shared_ptr<Helper> _helper;

	void alarm();

public:
	Timer() = delete; // Default-constructor
	Timer(const Timer&) = delete; // Copy-constructor
	Timer& operator=(Timer const&) = delete; // Copy-assignment
	Timer(Timer&&) noexcept = delete; // Move-constructor
	Timer& operator=(Timer&&) noexcept = delete; // Move-assignment

	explicit Timer(std::function<void()> handler, const char* label/* = "Timer"*/);
	~Timer() override = default;

	// Метка задачи (имя, название и т.п., для отладки)
	const char* label() const
	{
		return _label;
	}

	void startOnce(std::chrono::milliseconds duration);
	void restart(std::chrono::milliseconds duration);
	void stop();
};
