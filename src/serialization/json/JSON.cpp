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
#include "../SInt.hpp"
#include "../SFloat.hpp"
#include "../../utils/literals.hpp"

SVal* JSON::decode(std::string &data)
{
	std::istringstream iss(data);

	SVal* value = nullptr;

	try
	{
		value = decodeValue(iss);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't decode json: ") + exception.what());
	}

	// Проверяем лишние символы в конце
	skipSpaces(iss);
	if (!iss.eof())
	{
		throw std::runtime_error("Redundant data");
	}

	return value;
}

std::string JSON::encode(const SVal* value)
{
	std::ostringstream oss;

	try
	{
		encodeValue(value, oss);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't encode value: ") + exception.what());
	}

	return std::move(oss.str());
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

SVal* JSON::decodeValue(std::istringstream& iss)
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
			return decodeString(iss);

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

SObj* JSON::decodeObject(std::istringstream& iss)
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
			return obj.release();
		}

		auto key = std::unique_ptr<SStr>(decodeString(iss));

		skipSpaces(iss);

		c = iss.get();
		if (c != ':')
		{
			throw std::runtime_error("Wrong token");
		}

		skipSpaces(iss);

		obj->insert(key.release(), decodeValue(iss));

		skipSpaces(iss);

		c = iss.get();
		if (c == ',')
		{
			continue;
		}
		else if (c == '}')
		{
			return obj.release();
		}

		throw std::runtime_error("Wrong token");
	}

	throw std::runtime_error("Unexpect out of data");
}

SArr* JSON::decodeArray(std::istringstream& iss)
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
			return arr.release();
		}

		arr->insert(decodeValue(iss));

		skipSpaces(iss);

		c = iss.get();
		if (c == ',')
		{
			continue;
		}
		else if (c == ']')
		{
			return arr.release();
		}

		throw std::runtime_error("Wrong token");
	}

	throw std::runtime_error("Unexpect out of data");
}

SStr* JSON::decodeString(std::istringstream& iss)
{
	auto str = std::make_unique<SStr>();

	auto c = iss.get();
	if (c != '"')
	{
		throw std::runtime_error("Wrong token");
	}

	while (!iss.eof())
	{
		c = iss.peek();
		// Конец строки
		if (c == '"')
		{
			iss.ignore();
			return str.release();
		}
		// Экранированный сивол
		else if (c == '\\')
		{
			uint32_t symbol = decodeEscaped(iss);
			putUtf8Symbol(*str, symbol);
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
			iss.ignore();
			str->insert(static_cast<uint8_t>(c));
		}
		else
		{
			iss.ignore();
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
			while (--bytes)
			{
				if (iss.eof())
				{
					throw std::runtime_error("Unxpected end of data");
				}
				c = iss.get();
				if ((c & 0b11000000) != 0b10000000)
				{
					throw std::runtime_error("Bad symbol in string value");
				}
				chr = (chr << 6) | (c & 0b0011'1111);
			}
			putUtf8Symbol(*str, chr);
		}
	}

	throw std::runtime_error("Unexpect out of data");
}

SBool* JSON::decodeBool(std::istringstream& iss)
{
	auto c = iss.get();
	if (c == 't')
	{
		if (!iss.eof() && iss.get() == 'r')
			if (!iss.eof() && iss.get() == 'u')
				if (!iss.eof() && iss.get() == 'e')
					return new SBool(true);
	}
	else if (c == 'f')
	{
		if (!iss.eof() && iss.get() == 'a')
			if (!iss.eof() && iss.get() == 'l')
				if (!iss.eof() && iss.get() == 's')
					if (!iss.eof() && iss.get() == 'e')
						return new SBool(false);
	}
	if (iss.eof())
	{
		throw std::runtime_error("Unxpected end of data");
	}

	throw std::runtime_error("Wrong token");
}

SNull* JSON::decodeNull(std::istringstream& iss)
{
	if (!iss.eof() && iss.get() == 'n')
		if (!iss.eof() && iss.get() == 'u')
			if (!iss.eof() && iss.get() == 'l')
				if (!iss.eof() && iss.get() == 'l')
					return new SNull();
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
		else if (c >= 'a' && c <= 'f')
		{
			val = (val << 4) | ((c & 0x0F) - 'a');
		}
		else
		{
			throw std::runtime_error("Wrong token");
		}
	}

	return val;
}



SNum* JSON::decodeNumber(std::istringstream& iss)
{
	auto p = iss.tellg();

	while (!iss.eof())
	{
		auto c = iss.get();
		if (c == '.' || c == 'e' || c == 'E')
		{
			iss.seekg(p);
			long double value = 0;
			iss >> value;
			return new SFloat(value);
		}
	}

	iss.seekg(p);
	int64_t value = 0;
	iss >> value;
	return new SInt(value);
}

void JSON::encodeNull(const SNull *value, std::ostringstream &oss)
{
	oss << "null";
}

void JSON::encodeBool(const SBool *value, std::ostringstream &oss)
{
	oss << (value->value() ? "true" : "false");
}

void JSON::encodeString(const SStr *value, std::ostringstream &oss)
{
	oss << '"' << value->value() << '"';
}

void JSON::encodeNumber(const SNum *value, std::ostringstream &oss)
{
	const SInt *intVal = dynamic_cast<const SInt*>(value);
	if (intVal)
	{
		oss << intVal->value();
		return;
	}
	const SFloat *floatVal = dynamic_cast<const SFloat*>(value);
	if (floatVal)
	{
		oss << floatVal->value();
		return;
	}
}

void JSON::encodeArray(const SArr *value, std::ostringstream &oss)
{
	oss << "[";

	bool empty = true;
	value->forEach([&oss,&empty](const SVal* value){
		if (!empty)
		{
			oss << ",";
		}
		else
		{
			empty = false;
		}
		encodeValue(value, oss);
	});

	oss << "]";
}

void JSON::encodeObject(const SObj *value, std::ostringstream &oss)
{
	oss << "{";

	bool empty = true;
	value->forEach([&oss,&empty](const std::pair<const SStr* const, SVal*>&element){
		if (!empty)
		{
			oss << ",";
		}
		else
		{
			empty = false;
		}
		encodeString(element.first, oss);
		oss << ':';
		encodeValue(element.second, oss);
	});

	oss << "}";
}

void JSON::encodeValue(const SVal *value, std::ostringstream &oss)
{
	if (auto p = dynamic_cast<const SStr *>(value))
	{
		encodeString(p, oss);
	}
	else if (auto p = dynamic_cast<const SNum *>(value))
	{
		encodeNumber(p, oss);
	}
	else if (auto p = dynamic_cast<const SObj *>(value))
	{
		encodeObject(p, oss);
	}
	else if (auto p = dynamic_cast<const SArr *>(value))
	{
		encodeArray(p, oss);
	}
	else if (auto p = dynamic_cast<const SBool *>(value))
	{
		encodeBool(p, oss);
	}
	else if (auto p = dynamic_cast<const SNull *>(value))
	{
		encodeNull(p, oss);
	}
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}

void JSON::putUtf8Symbol(SStr &str, uint32_t symbol)
{
	if (symbol <= 0b0111'1111) // 7bit -> 1byte
	{
		str.insert(symbol);
	}
	else if (symbol <= 0b0111'1111'1111) // 11bit -> 2byte
	{
		str.insert(0b1100'0000 | (0b0001'1111 & (symbol>>6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>0)));
	}
	else if (symbol <= 0b1111'1111'1111'1111) // 16bit -> 3byte
	{
		str.insert(0b1110'0000 | (0b0000'1111 & (symbol>>12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>0)));
	}
	else if (symbol <= 0b0001'1111'1111'1111'1111'1111) // 21bit -> 4byte
	{
		str.insert(0b1111'0000 | (0b0000'0111 & (symbol>>18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>0)));
	}
	else if (symbol <= 0b0011'1111'1111'1111'1111'1111'1111) // 26bit -> 5byte
	{
		str.insert(0b1111'1000 | (0b0000'0011 & (symbol>>24)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>0)));
	}
	else if (symbol <= 0b0111'1111'1111'1111'1111'1111'1111'1111) // 31bit -> 6byte
	{
		str.insert(0b1111'1100 | (0b0000'0001 & (symbol>>30)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>24)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol>>0)));
	}
}
