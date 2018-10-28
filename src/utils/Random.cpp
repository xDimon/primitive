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
// File created on: 2018.10.21

// Random.cpp


#include "Random.hpp"

#include <random>
#include <chrono>

#if __cplusplus >= 201703L
std::string Random::generateSequence(const std::string_view& lookUpTable, size_t length)
#else
std::string Random::generateSequence(const std::string& lookUpTable, size_t length)
#endif
{
	std::string sequence;

	std::default_random_engine generator(static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));

	for (;;)
	{
		auto rnd = static_cast<uint64_t>(
			std::generate_canonical<long double, std::numeric_limits<long double>::digits>(generator) * std::numeric_limits<uint64_t>::max()
		);
		for (;;)
		{
			sequence.push_back(lookUpTable[rnd % 62]);
			if (sequence.length() >= length)
			{
				return sequence;
			}
			rnd /= 62;
			if (rnd < lookUpTable.length())
			{
				break;
			}
		}
	}

	return sequence;
}
