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
// File created on: 2017.03.29

// Base64.cpp


#include "Base64.hpp"

const std::string Base64::base64_chars(
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/"
);

const std::string& Base64::charset()
{
	return base64_chars;
}

inline static bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

// |2              |1              |0              |
// |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|

// |3          |2          |1          |0          |
// |5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|

std::string Base64::encode(const void *data_, size_t length)
{
	auto data = static_cast<const char *>(data_);

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (length-- > 0)
	{
		char_array_3[i++] = static_cast<unsigned char>(*(data++));
		if (i == 3)
		{
			char_array_4[0] =  (char_array_3[0] & static_cast<uint8_t>(0xFC)) >> 2;
			char_array_4[1] = ((char_array_3[0] & static_cast<uint8_t>(0x03)) << 4) + ((char_array_3[1] & static_cast<uint8_t>(0xF0)) >> 4);
			char_array_4[2] = ((char_array_3[1] & static_cast<uint8_t>(0x0F)) << 2) + ((char_array_3[2] & static_cast<uint8_t>(0xC0)) >> 6);
			char_array_4[3] = char_array_3[2] & static_cast<uint8_t>(0x3F);

			for (i = 0; (i < 4); i++)
			{
				ret += base64_chars[char_array_4[i]];
			}
			i = 0;
		}
	}

	if (i > 0)
	{
		for (j = i; j < 3; j++)
		{
			char_array_3[j] = '\0';
		}

		char_array_4[0] =  (char_array_3[0] & static_cast<uint8_t>(0xFC)) >> 2;
		char_array_4[1] = ((char_array_3[0] & static_cast<uint8_t>(0x03)) << 4) + ((char_array_3[1] & static_cast<uint8_t>(0xF0)) >> 4);
		char_array_4[2] = ((char_array_3[1] & static_cast<uint8_t>(0x0F)) << 2) + ((char_array_3[2] & static_cast<uint8_t>(0xC0)) >> 6);
		char_array_4[3] = char_array_3[2] & static_cast<uint8_t>(0x3F);

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

	while (length-- > 0 && (encoded_string[in_] != '='))
	{
		if (!is_base64(static_cast<unsigned char>(encoded_string[in_])))
		{
			in_++;
			continue;
		}

		char_array_4[i++] = static_cast<unsigned char>(encoded_string[in_]);
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
			}

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & static_cast<uint8_t>(0x30)) >> 4);
			char_array_3[1] = ((char_array_4[1] & static_cast<uint8_t>(0x0F)) << 4) + ((char_array_4[2] & static_cast<uint8_t>(0x3C)) >> 2);
			char_array_3[2] = ((char_array_4[2] & static_cast<uint8_t>(0x03)) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
			{
				ret += char_array_3[i];
			}
			i = 0;
		}
	}

	if (i > 0)
	{
		for (j = i; j < 4; j++)
		{
			char_array_4[j] = 0;
		}

		for (j = 0; j < 4; j++)
		{
			char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
		}

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & static_cast<uint8_t>(0x30)) >> 4);
		char_array_3[1] = ((char_array_4[1] & static_cast<uint8_t>(0x0F)) << 4) + ((char_array_4[2] & static_cast<uint8_t>(0x3C)) >> 2);
		char_array_3[2] = ((char_array_4[2] & static_cast<uint8_t>(0x03)) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
		{
			ret += char_array_3[j];
		}
	}

	return ret;
}
