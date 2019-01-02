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
// File created on: 2017.09.17

// LimitManager.hpp


#pragma once


#include <tuple>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "Limit.hpp"
#include "LimitConfig.hpp"
#include "../../utils/hash/SipHash.hpp"

class LimitManager
{
private:
	struct KeyHash
	{
		size_t operator()(const Limit::Id& key) const noexcept
		{
			static const char hidSeed[16]{};
			SipHash hash(hidSeed);
			hash(key);
			return hash.computeHash();
		}
	};

	std::unordered_map<
		Limit::Id,
		std::shared_ptr<const LimitConfig>,
		KeyHash
	> _configs;

	std::mutex _mutex;

public:
	LimitManager(const LimitManager&) = delete; // Copy-constructor
	LimitManager& operator=(LimitManager const&) = delete; // Copy-assignment
	LimitManager(LimitManager&&) noexcept = delete; // Move-constructor
	LimitManager& operator=(LimitManager&&) noexcept = delete; // Move-assignment

private:
	LimitManager() = default;
	~LimitManager() = default;

	static LimitManager &getInstance()
	{
		static LimitManager instance;
		return instance;
	}

public:
	static void push(
		const Limit::Id& id, // Id лимита
		Limit::Type type, // Тип поведения
		Limit::Value start, // Начальное значение
		Limit::Value max, // Максимальное значение
		uint32_t offset, // Сдвиг для фиксированных сбросов (Daily, Weekly, Monthly, Yearly)
		uint32_t duration // Продолжительность
	);

	static void remove(
		const Limit::Id& id // Id лимита
	);

	static std::shared_ptr<const LimitConfig> get(
		const Limit::Id& id
	);

	static void forEach(const std::function<void(const std::shared_ptr<const LimitConfig>&)>& handler);
};
