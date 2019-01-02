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
// File created on: 2018.10.31

// OTP.cpp


#include <sstream>
#include <iomanip>
#include "OTP.hpp"
#include "Random.hpp"
#include "encoding/Base32.hpp"
#include "Time.hpp"
#include "hash/HMAC_SHA1.hpp"

std::string OTP::generateSecret()
{
	return Random::generateSequence(Base32::charset(), 20);
}

std::string OTP::getHOTP(uint64_t count)
{
	auto T = htobe64(count);

	std::string msg;
	for (size_t i = 0; i < sizeof(T); i++)
	{
		msg.push_back(((uint8_t*)(&T))[i]);
	}

	auto hash = HMAC_SHA1(_key, msg);

	auto offsetBits = hash[hash.size()-1] & 0x0F;

	uint32_t truncated = (
		static_cast<uint32_t>(hash[offsetBits+0] & 0x7f) << 24 |
		static_cast<uint32_t>(hash[offsetBits+1] & 0xff) << 16 |
		static_cast<uint32_t>(hash[offsetBits+2] & 0xff) <<  8 |
		static_cast<uint32_t>(hash[offsetBits+3] & 0xff) <<  0
	) % 1000000;

	std::ostringstream oss;
	oss << std::setw(6) << std::setfill('0') << truncated;

	return oss.str();
}

std::string OTP::getTOTP()
{
	return getHOTP(static_cast<uint64_t>(Time::now() / 30));
}
