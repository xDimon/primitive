// Copyright © 2017 Dmitriy Khaustov
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
// File created on: 2017.10.07

// CRC32.hpp


#pragma once


#include <cinttypes>
#include <string>

class CRC32
{
private:
	uint32_t _crc32;

public:
	CRC32();
	CRC32(const void *data, size_t len);
	CRC32(const std::string& str)
	: CRC32(str.data(), str.length())
	{
	}
	~CRC32() = default;

	CRC32(const CRC32&) = delete;
	void operator= (CRC32 const&) = delete;

	void append(const void *data, size_t len);
	void append(const std::string& str)
	{
		return append(str.data(), str.length());
	}
	uint32_t getBytesHash();
	std::string getStringHash();

};
