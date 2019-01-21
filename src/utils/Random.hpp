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
// File created on: 2018.10.21

// Random.hpp


#pragma once

#include <string>
#if __cplusplus >= 201703L
#include <string_view>
#endif
#include <random>

class Random
{
public:
	Random(const Random&) = delete; // Copy-constructor
	Random& operator=(Random const&) = delete; // Copy-assignment
	Random(Random&&) noexcept = delete; // Move-constructor
	Random& operator=(Random&&) noexcept = delete; // Move-assignment

private:
	Random();
	~Random() = default;

public:
	static Random& getInstance()
	{
		static Random instance;
		return instance;
	}

private:
	std::default_random_engine _generator;

public:
#if __cplusplus >= 201703L
	static std::string generateSequence(std::string_view lookUpTable, size_t length);
#else
	static std::string generateSequence(const std::string& lookUpTable, size_t length);
#endif

	template <typename T>
	static T generate(const T& minValue, const T& maxValue)
	{
		return static_cast<T>(
			std::min(minValue, maxValue) +
			std::generate_canonical<long double, std::numeric_limits<long double>::digits>(getInstance()._generator)
			    * (std::max(minValue, maxValue) - std::min(minValue, maxValue))
		);
	}

};
