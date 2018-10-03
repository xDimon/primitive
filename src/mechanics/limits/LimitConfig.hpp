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
// File created on: 2017.09.17

// LimitConfig.hpp


#pragma once


#include "Limit.hpp"

class LimitConfig
{
public:
	const Limit::Id id; // Id лимита
	const Limit::Type type; // Тип поведения
	const Limit::Value start; // Начальное значение
	const Limit::Value max; // Максимальное значение
	const uint32_t offset; // Сдвиг для фиксированных сбросов (Day, Week, Month, Year)
	const uint32_t duration; // Продолжительность

public:
	LimitConfig(const LimitConfig&) = delete; // Copy-constructor
	void operator=(LimitConfig const&) = delete; // Copy-assignment
	LimitConfig(LimitConfig&&) = delete; // Move-constructor
	LimitConfig& operator=(LimitConfig&&) = delete; // Move-assignment

	LimitConfig(
		Limit::Id id, // Id лимита
		Limit::Type type, // Тип поведения
		Limit::Value start, // Начальное значение
		Limit::Value max, // Максимальное значение
		uint32_t offset, // Сдвиг для фиксированных сбросов (Day, Week, Month, Year)
		uint32_t duration // Продолжительность
	);

	~LimitConfig() = default;
};
