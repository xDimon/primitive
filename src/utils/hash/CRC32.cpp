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
// File created on: 2017.10.07

// CRC32.cpp


#include <sstream>
#include <iomanip>
#include "CRC32.hpp"

template<unsigned long t>
struct Polynome
{
	static const unsigned long value = (t & 1) ? ((t >> 1) ^ 0xEDB88320) : (t >> 1);
};

template<unsigned long t, int i>
struct For
{
	static const unsigned long value = For<Polynome<t>::value, i - 1>::value;
};

template<unsigned long t>
struct For<t, 0>
{
	static const unsigned long value = Polynome<t>::value;
};

template<unsigned long t>
struct Hash
{
	static const unsigned long value = For<t, 7>::value;
};

template<int r, int t>
struct Table : Table<r + 1, t - 1>
{
	Table()
	{
		Table<r + 1, t - 1>::values[t] = Hash<t>::value;
	}
};

template<int r>
struct Table<r, 0>
{
	int values[r + 1];

	Table()
	{
		values[0] = Hash<0>::value;
	}

	int operator[](int i)
	{
		return values[i];
	}
};

typedef Table<0,255> CRC_TABLE;

static CRC_TABLE crc_table;

CRC32::CRC32()
: _crc32(0xffffffff)
{
}

CRC32::CRC32(const void* data, size_t len)
: CRC32()
{
	append(data, len);
}

void CRC32::append(const void* data, size_t len)
{
	const char *buf = (const char *)(data);
	for (size_t i = 0; i < len; i++)
	{
		_crc32 = crc_table[(_crc32 ^ (*buf++)) & 0xff] ^ (_crc32 >> 8);
	}
}

uint32_t CRC32::getBytesHash()
{
	return _crc32 ^ 0xffffffff;
}

std::string CRC32::getStringHash()
{
	std::ostringstream oss;
	oss << std::setw(8) << std::setfill('0') << std::uppercase << std::hex << getBytesHash();
	return std::move(oss.str());
}
