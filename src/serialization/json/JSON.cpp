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
// File created on: 2017.04.07

// JSON.cpp


#include <sstream>
#include <cmath>
#include <memory>
#include "JSON.hpp"
#include "../../utils/literals.hpp"

SVal& JSON::decode(std::string &data)
{
	std::istringstream iss(data);

	SVal& value = decodeValue(iss);

	// Проверяем лишние символы в конце
	skipSpaces(iss);
	if (!iss.eof())
	{
		throw std::runtime_error("Redundant data");
	}

	return value;
}

std::string JSON::encode(SVal &obj)
{
//	std::ostringstream ;
	return "";
}

void JSON::skipSpaces(std::istringstream& iss)
{
	while (!iss.eof())
	{
		auto c = iss.peek();
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			return;
		}
		iss.ignore();
	}
}

SVal& JSON::decodeValue(std::istringstream& iss)
{
	skipSpaces(iss);

	switch (iss.peek())
	{
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return decodeNumber(iss);

		case '"':
		{
			return decodeString(iss);
		}

		case 't':
		case 'f':
			return decodeBool(iss);

		case '{':
			return decodeObject(iss);

		case '[':
			return decodeArray(iss);

		case 'n':
			return decodeNull(iss);

		default:
			throw std::runtime_error("Unknown token");
	}
}

SObj& JSON::decodeObject(std::istringstream& iss)
{
	auto obj = std::make_unique<SObj>();

	auto c = iss.get();
	if (c != '{')
	{
		throw std::runtime_error("Wrong token");
	}

	while (!iss.eof())
	{
		skipSpaces(iss);

		if (iss.peek() == '}')
		{
			iss.ignore();
			return *obj.release();
		}

		SStr& key = decodeString(iss);

		skipSpaces(iss);

		c = iss.get();
		if (c != ':')
		{
			throw std::runtime_error("Wrong token");
		}

		skipSpaces(iss);

		SVal& value = decodeValue(iss);

		obj->insert(key, value);

		skipSpaces(iss);

		c = iss.get();
		if (c == ',')
		{
			continue;
		}
		else if (c == '}')
		{
			return *obj.release();
		}

		throw std::runtime_error("Wrong token");
	}

	throw std::runtime_error("Unexpect out of data");
}

SArr& JSON::decodeArray(std::istringstream& iss)
{
	auto arr = std::make_unique<SArr>();

	auto c = iss.get();
	if (c != '[')
	{
		throw std::runtime_error("Wrong token");
	}

	while (!iss.eof())
	{
		skipSpaces(iss);

		if (iss.peek() == ']')
		{
			iss.ignore();
			return *arr.release();
		}

		SVal& value = decodeValue(iss);

		arr->insert(value);

		skipSpaces(iss);

		c = iss.get();
		if (c == ',')
		{
			continue;
		}
		else if (c == ']')
		{
			return *arr.release();
		}

		throw std::runtime_error("Wrong token");
	}

	throw std::runtime_error("Unexpect out of data");
}

SStr& JSON::decodeString(std::istringstream& iss)
{
	std::unique_ptr<SStr> str(new SStr);
//	auto str = std::make_unique<SStr>();

	auto c = iss.get();
	if (c != '"')
	{
		throw std::runtime_error("Wrong token");
	}

	while (!iss.eof())
	{
		c = iss.get();
		// Конец строки
		if (c == '"')
		{
			return *str.release();
		}
		// Экранированный сивол
		else if (c == '\\')
		{
			str->insert(decodeEscaped(iss));
		}
		// Управляющий символ
		else if (c < ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			throw std::runtime_error("Bad symbol in string value");
		}
		// Некорректный символ
		else if (c > 0b1111'1101)
		{
			throw std::runtime_error("Bad symbol in string value");
		}
		// Однобайтовый символ
		else if (c < 0b1000'0000)
		{
			str->insert(static_cast<uint8_t>(c));
		}
		else
		{
			int bytes;
			uint32_t chr = 0;
			if ((c & 0b11111100) == 0b11111100)
			{
				bytes = 6;
				chr = static_cast<uint8_t>(c) & 0b1_u8;
			}
			else if ((c & 0b11111000) == 0b11111000)
			{
				bytes = 5;
				chr = static_cast<uint8_t>(c) & 0b11_u8;
			}
			else if ((c & 0b11110000) == 0b11110000)
			{
				bytes = 4;
				chr = static_cast<uint8_t>(c) & 0b111_u8;
			}
			else if ((c & 0b11100000) == 0b11100000)
			{
				bytes = 3;
				chr = static_cast<uint8_t>(c) & 0b1111_u8;
			}
			else if ((c & 0b11000000) == 0b11000000)
			{
				bytes = 2;
				chr = static_cast<uint8_t>(c) & 0b11111_u8;
			}
			else
			{
				throw std::runtime_error("Bad symbol in string value");
			}
			while (bytes--)
			{
				if (iss.eof())
				{
					throw std::runtime_error("Unxpected end of data");
				}
				c = iss.get();
				if ((c & 0b11000000) != 0b11000000)
				{
					throw std::runtime_error("Bad symbol in string value");
				}
				chr = (chr << 6) | (c & 0b0011'1111);
			}
			str->insert(chr);
		}
	}

	throw std::runtime_error("Unexpect out of data");
}

SBool& JSON::decodeBool(std::istringstream& iss)
{
	auto c = iss.get();
	if (c == 't')
	{
		if (!iss.eof() && iss.get() == 'r')
			if (!iss.eof() && iss.get() == 'u')
				if (!iss.eof() && iss.get() == 'e')
					return *new SBool(true);
	}
	else if (c == 'f')
	{
		if (!iss.eof() && iss.get() == 'a')
			if (!iss.eof() && iss.get() == 'l')
				if (!iss.eof() && iss.get() == 's')
					if (!iss.eof() && iss.get() == 'e')
						return *new SBool(false);
	}
	if (iss.eof())
	{
		throw std::runtime_error("Unxpected end of data");
	}

	throw std::runtime_error("Wrong token");
}

SNull& JSON::decodeNull(std::istringstream& iss)
{
	if (!iss.eof() && iss.get() == 'n')
		if (!iss.eof() && iss.get() == 'u')
			if (!iss.eof() && iss.get() == 'l')
				if (!iss.eof() && iss.get() == 'l')
					return *new SNull();
	if (iss.eof())
	{
		throw std::runtime_error("Unxpected end of data");
	}

	throw std::runtime_error("Wrong token");
}

uint32_t JSON::decodeEscaped(std::istringstream& iss)
{
	auto c = iss.get();
	if (c != '\\')
	{
		throw std::runtime_error("Isn't escaped symbol");
	}
	if (iss.eof())
	{
		throw std::runtime_error("Unxpected end of data");
	}
	c = iss.get();
	switch (c)
	{
		case '"': return '"';
		case '\\': return '\\';
		case '/': return '/';
		case 'b': return '\b';
		case 'f': return '\f';
		case 'n': return '\n';
		case 't': return '\t';
		case 'r': return '\r';
		case 'u': break;
		default: throw std::runtime_error("Wrong token");
	}
	uint32_t val = 0;
	for (int i = 0; i < 4; i++)
	{
		if (iss.eof())
		{
			throw std::runtime_error("Unxpected end of data");
		}
		c = iss.get();
		if (c >= '0' && c <= '9')
		{
			val = (val << 4) | ((c & 0x0F) - '0');
		}
		else if (c >= 'A' && c <= 'F')
		{
			val = (val << 4) | ((c & 0x0F) - 'A');
		}
		else
		{
			throw std::runtime_error("Wrong token");
		}
	}

	return val;
}

SNum& JSON::decodeNumber(std::istringstream& iss)
{
	bool negInt = false;
	int64_t integer = 0;
	bool negExp = false;
	int64_t exp0 = 0;
	int64_t exp = 0;

	if (iss.peek() == '-')
	{
		negInt = true;
		iss.ignore();
	}
	if (iss.eof())
	{
		throw std::runtime_error("Unxpected end of data");
	}

	while (!iss.eof())
	{
		auto c = iss.peek();
		if (c >= '0' && c <= '9')
		{
			integer = integer * 10 + (c - '0');
			iss.ignore();
		}
		else if (c == '.')
		{
			iss.ignore();
			break;
		}
		else if (c == 'e' || c == 'E')
		{
			break;
		}
		else if (c != ',' && c != '}' && c != ']' && c != ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			throw std::runtime_error("Wrong token");
		}
		else
		{
			return *new SNum((negInt ? -1 : 1) * integer);
		}
	}

	while (!iss.eof())
	{
		auto c = iss.peek();
		if (c >= '0' && c <= '9')
		{
			exp0--;
			integer = integer * 10 + (c - '0');
			iss.ignore();
		}
		else if (c == 'e' || c == 'E')
		{
			iss.ignore();
			break;
		}
		else if (c != ',' && c != '}' && c != ']' && c != ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			throw std::runtime_error("Wrong token");
		}
		else
		{
			return *new SNum((negInt ? -1 : 1) * pow10(exp0) * integer);
		}
	}

	if (iss.peek() == '-')
	{
		negExp = true;
		iss.ignore();
	}
	else if (iss.peek() == '+')
	{
		iss.ignore();
	}

	while (!iss.eof())
	{
		auto c = iss.peek();
		if (c >= '0' && c <= '9')
		{
			exp = exp * 10 + (c - '0');
			iss.ignore();
		}
		else if (c != ',' && c != '}' && c != ']' && c != ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			throw std::runtime_error("Wrong token");
		}
		else
		{
			break;
		}
	}

	return *new SNum((negInt ? -1 : 1) * pow10(exp0 + (negExp ? -1 : 1) * exp) * integer);
}
