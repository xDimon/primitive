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
// File created on: 2017.03.29

// Base64.cpp


#include "Base64.hpp"

#include "literals.hpp"

static const std::string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

inline static bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Base64::encode(const char *data, size_t length)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (length--)
	{
		char_array_3[i++] = static_cast<unsigned char>(*(data++));
		if (i == 3)
		{
			char_array_4[0] =  (char_array_3[0] & 0xFC_u8) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03_u8) << 4) + ((char_array_3[1] & 0xF0_u8) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0F_u8) << 2) + ((char_array_3[2] & 0xC0_u8) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3F_u8;

			for (i = 0; (i < 4); i++)
			{
				ret += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
		{
			char_array_3[j] = '\0';
		}

		char_array_4[0] =  (char_array_3[0] & 0xFC_u8) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03_u8) << 4) + ((char_array_3[1] & 0xF0_u8) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0F_u8) << 2) + ((char_array_3[2] & 0xC0_u8) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3F_u8;

		for (j = 0; (j < i + 1); j++)
		{
			ret += base64_chars[char_array_4[j]];
		}

		while ((i++ < 3))
		{
			ret += '=';
		}
	}

	return ret;
}

std::string Base64::decode(std::string const &encoded_string)
{
	size_t length = encoded_string.size();
	size_t i = 0;
	size_t j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (length-- && (encoded_string[in_] != '='))
	{
		if (!is_base64(encoded_string[in_]))
		{
			in_++;
			continue;
		}

		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30_u8) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0x0F_u8) << 4) + ((char_array_4[2] & 0x3C_u8) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x03_u8) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
			{
				ret += char_array_3[i];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
		{
			char_array_4[j] = 0;
		}

		for (j = 0; j < 4; j++)
		{
			char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
		}

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30_u8) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0x0F_u8) << 4) + ((char_array_4[2] & 0x3C_u8) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x03_u8) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
		{
			ret += char_array_3[j];
		}
	}

	return ret;
}
