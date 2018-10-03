// Copyright © 2017-2018 Dmitriy Khaustov
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
// File created on: 2017.03.28

// HttpHelper.hpp


#pragma once

#include <cstdint>

namespace HttpHelper
{

inline bool isChar(char c)
{
	return static_cast<uint8_t>(c) <= 0x7F;
}
inline bool isUpalpha(char c)
{
	return static_cast<uint8_t>(c) >= 'A' && static_cast<uint8_t>(c) <= 'Z';
}
inline bool isLoalpha(char c)
{
	return static_cast<uint8_t>(c) >= 'a' && static_cast<uint8_t>(c) <= 'z';
}
inline bool isAlpha(char c)
{
	return isUpalpha(c) || isLoalpha(c);
}
inline bool isDigit(char c)
{
	return static_cast<uint8_t>(c) >= '0' && static_cast<uint8_t>(c) <= '9';
}
inline bool isCtl(char c)
{
	return static_cast<uint8_t>(c) < ' ' || static_cast<uint8_t>(c) == 0x7F;
}
inline bool isCr(char c)
{
	return static_cast<uint8_t>(c) == '\r';
}
inline bool isLf(char c)
{
	return static_cast<uint8_t>(c) == '\n';
}
inline bool isSp(char c)
{
	return static_cast<uint8_t>(c) == ' ';
}
inline bool isHt(char c)
{
	return static_cast<uint8_t>(c) == '\t';
}
inline bool isQuote(char c)
{
	return static_cast<uint8_t>(c) == '"';
}
inline bool isHex(char c)
{
	return
		isDigit(c)
		|| (static_cast<uint8_t>(c) >= 'A' && static_cast<uint8_t>(c) <= 'F')
		|| (static_cast<uint8_t>(c) >= 'a' && static_cast<uint8_t>(c) <= 'f')
		;
}
inline bool isSpecial(char c)
{
	return static_cast<uint8_t>(c) == '(' || static_cast<uint8_t>(c) == ')'
		|| static_cast<uint8_t>(c) == '<' || static_cast<uint8_t>(c) == '>'
		|| static_cast<uint8_t>(c) == '[' || static_cast<uint8_t>(c) == ']'
		|| static_cast<uint8_t>(c) == '{' || static_cast<uint8_t>(c) == '}'
		|| static_cast<uint8_t>(c) == ' ' || static_cast<uint8_t>(c) == '\t'
		|| static_cast<uint8_t>(c) == '/' || static_cast<uint8_t>(c) == '\\'
		|| static_cast<uint8_t>(c) == ','
		|| static_cast<uint8_t>(c) == ';'
		|| static_cast<uint8_t>(c) == ':'
		|| static_cast<uint8_t>(c) == '"'
		|| static_cast<uint8_t>(c) == '@'
		|| static_cast<uint8_t>(c) == '?'
		|| static_cast<uint8_t>(c) == '='
		;
}
inline bool isToken(char c)
{
	return !isSpecial(c) && !isCtl(c);
}
inline bool isCrlf(const char *c)
{
	return isCr(c[0]) && isLf(c[1]);
}
inline bool isLws(const char *c)
{
	return isSp(c[0]) || isHt(c[0]) || isCrlf(c);
}
inline char* skipLws(char *c)
{
	for (;;)
	{
		if (isSp(*c) || isHt(*c))
		{
			c++;
			continue;
		}
		if (isCrlf(c))
		{
			c += 2;
			continue;
		}
		break;
	}
	return c;
}
inline const char* skipLws(const char *c)
{
	return skipLws(const_cast<char *>(c));
}

};
