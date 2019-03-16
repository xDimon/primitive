// Copyright © 2019 Dmitriy Khaustov
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
// File created on: 2019.03.04

// HMAC_SHA256.cpp


#include <vector>
#include "HMAC_SHA256.hpp"
#include "SHA256.hpp"

#define SHA256_BLOCKSIZE 64

std::string HMAC_SHA256(const std::string& key, const std::string& msg)
{
	std::string key0;

	if (key.size() > SHA256_BLOCKSIZE)
	{
		auto hash = SHA256::encode_bin(key.data(), key.size());
		size_t i;
		for (i = 0; i < hash.size(); i++)
		{
			key0.push_back(hash.at(i));
		}
		for (i = hash.size(); i < SHA256_BLOCKSIZE; i++)
		{
			key0.push_back(0);
		}
	}
	else if (key.size() <= SHA256_BLOCKSIZE)
	{
		size_t i;
		for (i = 0; i < key.size(); i++)
		{
			key0.push_back(key.at(i));
		}
		for (i = key.size(); i < SHA256_BLOCKSIZE; i++)
		{
			key0.push_back(0);
		}
	}

//	std::string Si;
//	std::string So;
//
//	for (size_t i = 0; i < SHA256_BLOCKSIZE; i++)
//	{
//		Si.push_back(static_cast<uint8_t>(key0[i] ^ 0x36));
//		So.push_back(static_cast<uint8_t>(key0[i] ^ 0x5c));
//	}
//
//
//	auto Hi = SHA256::encode_bin(Si + msg);
//	auto Ho = SHA256::encode(So + Hi);

	std::vector<uint8_t> Si;
	std::vector<uint8_t> So;

	for (size_t i = 0; i < SHA256_BLOCKSIZE; i++)
	{
		Si.push_back(static_cast<uint8_t>(key0[i] ^ 0x36));
		So.push_back(static_cast<uint8_t>(key0[i] ^ 0x5c));
	}

	for (uint8_t m : msg)
	{
		Si.push_back(m);
	}

	auto Hi = SHA256::encode_bin(Si.data(), Si.size());

	for (uint8_t hi : Hi)
	{
		So.push_back(hi);
	}

	return SHA256::encode(So.data(), So.size());
}
