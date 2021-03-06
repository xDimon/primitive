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

// Random.cpp


#include "Random.hpp"

#include <random>
#include <chrono>
#include <stdexcept>

const std::string Random::lowerAlpha("abcdefghjiklmnopqrstuvwxyz");
const std::string Random::upperAlpha("ABCDEFGHJIKLMNOPQRSTUVWXYZ");
const std::string Random::digits("0123456789");
const std::string Random::alpha(lowerAlpha + upperAlpha);
const std::string Random::alphaAndDigits(alpha + digits);

Random::Random()
: _generator(static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()))
{
}

#if __cplusplus >= 201703L
std::string Random::generateSequence(std::string_view lookUpTable, size_t length)
#else
std::string Random::generateSequence(const std::string& lookUpTable, size_t length)
#endif
{
	if (lookUpTable.empty())
	{
		throw std::runtime_error("lookUpTable mustn't be empty!");
	}

	std::string sequence;

	for (;;)
	{
		auto rnd = static_cast<uint64_t>(
			std::generate_canonical<long double, std::numeric_limits<long double>::digits>(getInstance()._generator)
			    * std::numeric_limits<uint64_t>::max()
		);
		for (;;)
		{
			sequence.push_back(lookUpTable[rnd % lookUpTable.size()]);
			if (sequence.length() >= length)
			{
				return sequence;
			}
			rnd /= lookUpTable.size();
			if (rnd < lookUpTable.length())
			{
				break;
			}
		}
	}
}
