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
// File created on: 2018.10.30

// Base32.cpp


#include "Base32.hpp"

const std::string Base32::base32_chars(
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"234567"
);

const std::string& Base32::charset()
{
	return base32_chars;
}

inline static bool is_base32(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7');
}

// |0              |1              |2              |3              |4              |
// |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|

// |0        |1        |2        |3        |4        |5        |6        |7        |
// |4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|4 3 2 1 0|

std::string Base32::encode(const void *data_, size_t length)
{
	auto data = static_cast<const char *>(data_);

	std::string ret;
	size_t i = 0;
	size_t j = 0;
	unsigned char char_array_5[5];
	unsigned char char_array_8[8];

	while (length-- > 0)
	{
		char_array_5[i++] = static_cast<unsigned char>(*(data++));
		if (i == 5)
		{
			char_array_8[0] = ((char_array_5[0]>>3) & static_cast<uint8_t>(0x1F));
			char_array_8[1] = ((char_array_5[0]<<2) & static_cast<uint8_t>(0x1F)) | ((char_array_5[1]>>6) & static_cast<uint8_t>(0x1F));
			char_array_8[2] = ((char_array_5[1]>>1) & static_cast<uint8_t>(0x1F));
			char_array_8[3] = ((char_array_5[1]<<4) & static_cast<uint8_t>(0x1F)) | ((char_array_5[2]>>4) & static_cast<uint8_t>(0x1F));
			char_array_8[4] = ((char_array_5[2]<<1) & static_cast<uint8_t>(0x1F)) | ((char_array_5[3]>>7) & static_cast<uint8_t>(0x1F));
			char_array_8[5] = ((char_array_5[3]>>2) & static_cast<uint8_t>(0x1F));
			char_array_8[6] = ((char_array_5[3]<<3) & static_cast<uint8_t>(0x1F)) | ((char_array_5[4]>>5) & static_cast<uint8_t>(0x1F));
			char_array_8[7] = ((char_array_5[4]<<0) & static_cast<uint8_t>(0x1F));

			for (i = 0; i < 8; i++)
			{
				ret += base32_chars[char_array_8[i]];
			}
			i = 0;
		}
	}

	if (i > 0)
	{
		for (j = i; j < 5; j++)
		{
			char_array_5[j] = '\0';
		}

		char_array_8[0] = ((char_array_5[0]>>3) & static_cast<uint8_t>(0x1F));
		char_array_8[1] = ((char_array_5[0]<<2) & static_cast<uint8_t>(0x1F)) | ((char_array_5[1]>>6) & static_cast<uint8_t>(0x1F));
		char_array_8[2] = ((char_array_5[1]>>1) & static_cast<uint8_t>(0x1F));
		char_array_8[3] = ((char_array_5[1]<<4) & static_cast<uint8_t>(0x1F)) | ((char_array_5[2]>>4) & static_cast<uint8_t>(0x1F));
		char_array_8[4] = ((char_array_5[2]<<1) & static_cast<uint8_t>(0x1F)) | ((char_array_5[3]>>7) & static_cast<uint8_t>(0x1F));
		char_array_8[5] = ((char_array_5[3]>>2) & static_cast<uint8_t>(0x1F));
		char_array_8[6] = ((char_array_5[3]<<3) & static_cast<uint8_t>(0x1F)) | ((char_array_5[4]>>5) & static_cast<uint8_t>(0x1F));
		char_array_8[7] = ((char_array_5[4]<<0) & static_cast<uint8_t>(0x1F));

		size_t l[] = {0, 2, 4, 5, 7};

		for (j = 0; j < l[i]; j++)
		{
			ret += base32_chars[char_array_8[j]];
		}

		for (j = l[i]; j < 8; j++)
		{
			ret += '=';
		}
	}

	return ret;
}

std::string Base32::decode(std::string const &encoded_string)
{
	size_t length = encoded_string.size();
	size_t i = 0;
	size_t j = 0;
	int in_ = 0;
	unsigned char char_array_8[8], char_array_5[5];
	std::string ret;

	while (length-- > 0 && (encoded_string[in_] != '='))
	{
		if (!is_base32(static_cast<unsigned char>(encoded_string[in_])))
		{
			in_++;
			continue;
		}

		char_array_8[i++] = static_cast<unsigned char>(encoded_string[in_]);
		in_++;
		if (i == 8)
		{
			for (i = 0; i < 8; i++)
			{
				char_array_8[i] = static_cast<unsigned char>(base32_chars.find(char_array_8[i]));
			}

			char_array_5[0] = (char_array_8[0]<<3) | (char_array_8[1]>>2);
			char_array_5[1] = (char_array_8[1]<<6) | (char_array_8[2]<<1) | (char_array_8[3]>>4);
			char_array_5[2] = (char_array_8[3]<<4) | (char_array_8[4]>>1);
			char_array_5[3] = (char_array_8[4]<<7) | (char_array_8[5]<<2) | (char_array_8[6]>>3);
			char_array_5[4] = (char_array_8[6]<<5) | (char_array_8[7]>>0);

			for (i = 0; i < 5; i++)
			{
				ret += char_array_5[i];
			}
			i = 0;
		}
	}

	if (i > 0)
	{
		for (j = i; j < 8; j++)
		{
			char_array_8[j] = 0;
		}

		for (j = 0; j < 8; j++)
		{
			char_array_8[j] = static_cast<unsigned char>(base32_chars.find(char_array_8[j]));
		}

		char_array_5[0] = (char_array_8[0]<<3) | (char_array_8[1]>>2);
		char_array_5[1] = (char_array_8[1]<<6) | (char_array_8[2]<<1) | (char_array_8[3]>>4);
		char_array_5[2] = (char_array_8[3]<<4) | (char_array_8[4]>>1);
		char_array_5[3] = (char_array_8[4]<<7) | (char_array_8[5]<<2) | (char_array_8[6]>>3);
		char_array_5[4] = (char_array_8[6]<<5) | (char_array_8[7]>>0);

		size_t l[] = {0, 0, 1, 0, 2, 3, 0, 4};

		for (j = 0; j < l[i]; j++)
		{
			ret += char_array_5[j];
		}
	}

	return ret;
}
