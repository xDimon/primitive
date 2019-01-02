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

// HMAC_SHA1.cpp


#include "HMAC_SHA1.hpp"
#include "SHA1.hpp"

#define SHA1_BLOCKSIZE 64

std::string HMAC_SHA1(const std::string& key, const std::string& msg)
{
	std::string key0;

	if (key.size() > SHA1_BLOCKSIZE)
	{
		auto hash = SHA1::encode_bin(key);
		size_t i;
		for (i = 0; i < hash.size(); i++)
		{
			key0.push_back(hash.at(i));
		}
		for (i = hash.size(); i < SHA1_BLOCKSIZE; i++)
		{
			key0.push_back(0);
		}
	}
	else if (key.size() <= SHA1_BLOCKSIZE)
	{
		size_t i;
		for (i = 0; i < key.size(); i++)
		{
			key0.push_back(key.at(i));
		}
		for (i = key.size(); i < SHA1_BLOCKSIZE; i++)
		{
			key0.push_back(0);
		}
	}

	std::string Si;
	std::string So;

	for (size_t i = 0; i < SHA1_BLOCKSIZE; i++)
	{
		Si.push_back(static_cast<uint8_t>(key0[i] ^ 0x36));
		So.push_back(static_cast<uint8_t>(key0[i] ^ 0x5c));
	}

	SHA1 Hi;
	Hi.update(Si);
	Hi.update(msg);

	SHA1 Ho;
	Ho.update(So);
	Ho.update(Hi.final_bin());

	return Ho.final_bin();
}
