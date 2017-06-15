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
// File created on: 2012.09.06

// MD5.hpp


#pragma once

#include "stddef.h"
#include "stdint.h"
#include <string>

typedef uint8_t md5_byte_t; /* 8-bit byte */
typedef uint32_t md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_state_t;

class MD5
{
public:
	static const size_t BytesHashLength = 16;
	static const size_t StringHashLength = BytesHashLength<<1;

private:
	md5_state_t _state;
	md5_byte_t _digest[BytesHashLength];
	char _str_digest[StringHashLength+1];
	bool _finalized;

public:
	MD5();
	MD5(const void *data, size_t len);
	MD5(const std::string& str)
	: MD5(str.data(), str.length())
	{
	}
	virtual ~MD5();

	MD5(const MD5&) = delete;
	void operator= (MD5 const&) = delete;

	bool append(const void *data, size_t len);
	bool append(const std::string str)
	{
		return append(str.data(), str.length());
	}
	const char * getBytesHash();
	const char * getStringHash();
};
