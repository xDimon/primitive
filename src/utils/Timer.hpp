// Copyright © 2018-2019 Dmitriy Khaustov
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
public:
	typedef std::chrono::steady_clock::time_point AlarmTime;

private:
	class Helper final: public Shareable<Helper>
	{
	private:
		std::weak_ptr<Timer> _wp;
		size_t _refCounter;

		void onTime();

	public:
		Helper() = delete; // Default-constructor
		Helper(const Helper&) = delete; // Copy-constructor
		Helper& operator=(const Helper&) = delete; // Copy-assignment
		Helper(Helper&&) noexcept = delete; // Move-constructor
		Helper& operator=(Helper&&) noexcept = delete; // Move-assignment

		explicit Helper(const std::shared_ptr<Timer>& timer);
		~Helper() override = default;

		void start(AlarmTime time);

		void cancel();
	};


	using mutex_t = std::mutex;
	mutex_t _mutex;

	const char *_label;
	std::function<void()> _handler;

	AlarmTime _actualAlarmTime;
	AlarmTime _nextAlarmTime;

	std::shared_ptr<Helper> _helper;

	AlarmTime appoint(AlarmTime currentTime, AlarmTime alarmTime, bool once);
	bool alarm();

public:
	Timer() = delete; // Default-constructor
	Timer(const Timer&) = delete; // Copy-constructor
	Timer& operator=(const Timer&) = delete; // Copy-assignment
	Timer(Timer&&) noexcept = delete; // Move-constructor
	Timer& operator=(Timer&&) noexcept = delete; // Move-assignment

	explicit Timer(std::function<void()> handler, const char* label);
	~Timer() override = default;

	// Метка задачи (имя, название и т.п., для отладки)
	const char* label() const
	{
		return _label;
	}

	bool isActive() const
	{
		return _helper != nullptr;
	}

	// Запустить
	AlarmTime start(std::chrono::microseconds duration, bool once);

	// Запустить, только если не запущено
	AlarmTime startOnce(std::chrono::microseconds duration);
	// Перезапустить
	AlarmTime restart(std::chrono::microseconds duration);

	// Продлить
	AlarmTime prolong(std::chrono::microseconds duration);
	// Сократить
	AlarmTime shorten(std::chrono::microseconds duration);

	// Остановить
	void stop();
};
