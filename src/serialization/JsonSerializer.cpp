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
// File created on: 2017.04.07

// JsonSerializer.cpp


#include "JsonSerializer.hpp"
#include "../utils/encoding/Base64.hpp"

#include <sstream>
#include <iomanip>

REGISTER_SERIALIZER(json, JsonSerializer);

SVal JsonSerializer::decode(std::istream& is)
{
	if (is.eof() || is.peek() == -1)
	{
		throw JsonParseExeption("No data for parsing");
	}

	SVal value;

	try
	{
		value = decodeValue(is);
	}
	catch (std::exception& exception)
	{
		throw JsonParseExeption(std::string("Can't decode JSON ← ") + exception.what());
	}

	// Check for redundant data at end
	skipSpaces(is);

	if (!is.eof() && (_flags & Serializer::STRICT) != 0)
	{
		throw JsonParseExeption("Redundant bytes after parsed data", is.tellg(), is);
	}

	return value;
}

void JsonSerializer::encode(std::ostream &os, const SVal& value)
{
	try
	{
		encodeValue(os, value);
	}
	catch (std::exception& exception)
	{
		throw JsonParseExeption(std::string("Can't encode into JSON ← ") + exception.what());
	}
}

void JsonSerializer::skipSpaces(std::istream& is)
{
	while (std::isspace(is.peek()))
	{
		is.ignore();
	}
}

SVal JsonSerializer::decodeValue(std::istream& is)
{
	skipSpaces(is);

	if (is.eof())
	{
		throw JsonParseExeption("Unexpected out of data during parse value", is.tellg());
	}

	switch (is.peek())
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
			return decodeNumber(is);

		case '"':
		{
			auto p = is.tellg();
			if (
				is.get() == '"' &&
				is.get() == '=' &&
				is.get() == '?' &&
				is.get() == 'B' &&
				is.get() == '?'
			)
			{
				is.clear(std::istream::goodbit);
				is.seekg(p);
				return decodeBinary(is);
			}
			is.clear(std::istream::goodbit);
			is.seekg(p);
			return decodeString(is);
		}

		case 't':
		case 'f':
			return decodeBool(is);

		case '{':
			return decodeObject(is);

		case '[':
			return decodeArray(is);

		case 'n':
			return decodeNull(is);

		default:
			throw JsonParseExeption("Unknown token at parse value", is.tellg(), is);
	}
}

SVal JsonSerializer::decodeObject(std::istream& is)
{
	if (is.peek() != '{')
	{
		throw JsonParseExeption("Wrong token for open object", is.tellg(), is);
	}
	is.ignore();

	skipSpaces(is);

	SObj obj;

	if (is.peek() == '}')
	{
		is.ignore();
		return std::move(obj);
	}

	while (!is.eof())
	{
		std::string key;
		try
		{
			key = decodeString(is).as<SStr>().value();
		}
		catch (const std::runtime_error& exception)
		{
			throw JsonParseExeption(std::string("Can't parse field-key of object ← ") + exception.what());
		}

		skipSpaces(is);

		if (is.peek() != ':')
		{
			throw JsonParseExeption("Wrong token after field-key of object", is.tellg(), is);
		}
		is.ignore();

		skipSpaces(is);

		try
		{
			obj.emplace(std::move(key), decodeValue(is));
		}
		catch (const std::runtime_error& exception)
		{
			throw JsonParseExeption(std::string("Can't parse field-value of object ← ") + exception.what());
		}

		skipSpaces(is);

		if (is.peek() == ',')
		{
			is.ignore();
			skipSpaces(is);
			continue;
		}

		if (is.peek() == '}')
		{
			is.ignore();
			return std::move(obj);
		}

		throw JsonParseExeption("Wrong token after field-value of object", is.tellg(), is);
	}

	throw JsonParseExeption("Unexpected out of data during parse object", is.tellg());
}

SVal JsonSerializer::decodeArray(std::istream& is)
{
	if (is.peek() != '[')
	{
		throw JsonParseExeption("Wrong token for open array", is.tellg(), is);
	}
	is.ignore();

	skipSpaces(is);

	SArr arr;

	if (is.peek() == ']')
	{
		is.ignore();
		return std::move(arr);
	}

	while (!is.eof())
	{
		try
		{
			arr.emplace_back(decodeValue(is));
		}
		catch (const std::runtime_error& exception)
		{
			throw JsonParseExeption(std::string("Can't parse element of array ← ") + exception.what());
		}

		skipSpaces(is);

		if (is.peek() == ',')
		{
			is.ignore();
			skipSpaces(is);
			continue;
		}

		if (is.peek() == ']')
		{
			is.ignore();
			return std::move(arr);
		}

		throw JsonParseExeption("Wrong token after element of array", is.tellg(), is);
	}

	throw JsonParseExeption("Unexpected out of data during parse array", is.tellg());
}

SVal JsonSerializer::decodeString(std::istream& is)
{
	if (is.peek() != '"')
	{
		throw JsonParseExeption("Wrong token for open string", is.tellg(), is);
	}
	is.ignore();

	SStr str;

	while (!is.eof())
	{
		auto pos = is.tellg();
		auto c = is.peek();

		// Конец строки
		if (c == '"')
		{
			is.ignore();
			return std::move(str);
		}

		// Экранированный сивол
		if (c == '\\')
		{
			uint32_t symbol = decodeEscaped(is);

			if (symbol >= 0xD800 && symbol <= 0xDFFF) // Суррогатная пара UTF16
			{
				if ((symbol & 0b1111'1100'0000'0000) != 0b1101'1000'0000'0000)
				{
					throw JsonParseExeption("Bad escaped symbol: wrong for first byte of utf-16 surrogate pair", pos);
				}
				uint32_t symbol2 = decodeEscaped(is);
				if ((symbol2 & 0b1111'1100'0000'0000) != 0b1101'1100'0000'0000)
				{
					throw JsonParseExeption("Bad escaped symbol: wrong for second byte of utf-16 surrogate pair", pos);
				}
				symbol = ((((symbol >> 6) & 0b1111) + 1) << 16) | ((symbol & 0b11'1111) << 10) | (symbol2 & 0b0011'1111'1111);
			}

			putUnicodeSymbolAsUtf8(str, symbol);
		}
		// Управляющий символ
		else if (c < ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\b' && c != '\v')
		{
			throw JsonParseExeption("Bad symbol in string value: control symbol", pos);
		}
		// Некорректный символ
		else if (c > 0b1111'1101)
		{
			throw JsonParseExeption("Bad symbol in string value: prohibited utf8 byte", pos);
		}
		// Однобайтовый символ
		else if (c < 0b1000'0000)
		{
			is.ignore();
			str.push_back(static_cast<uint8_t>(c));
		}
		else
		{
			is.ignore();
			int bytes;
			uint32_t chr = 0;
			if ((c & 0b11111100) == 0b11111100)
			{
				bytes = 6;
				chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1);
			}
			else if ((c & 0b11111000) == 0b11111000)
			{
				bytes = 5;
				chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11);
			}
			else if ((c & 0b11110000) == 0b11110000)
			{
				bytes = 4;
				chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b111);
			}
			else if ((c & 0b11100000) == 0b11100000)
			{
				bytes = 3;
				chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1111);
			}
			else if ((c & 0b11000000) == 0b11000000)
			{
				bytes = 2;
				chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11111);
			}
			else
			{
				throw JsonParseExeption("Bad symbol in string value: invalid utf8 byte", pos);
			}
			while (--bytes > 0)
			{
				if (is.eof())
				{
					throw JsonParseExeption("Unxpected end of data during parse utf8 symbol", is.tellg());
				}

				c = is.peek();
				if ((c & 0b11000000) != 0b10000000)
				{
					throw JsonParseExeption("Bad symbol in string value: invalid utf8 byte", is.tellg());
				}
				is.ignore();

				chr = (chr << 6) | (c & 0b0011'1111);
			}
			putUnicodeSymbolAsUtf8(str, chr);
		}
	}

	throw JsonParseExeption("Unexpected out of data during parse string value", is.tellg());
}

SVal JsonSerializer::decodeBinary(std::istream& is)
{
	auto pos = is.tellg();
	if (is.get() != '"' ||
		is.get() != '=' ||
		is.get() != '?' ||
		is.get() != 'B' ||
		is.get() != '?'
	)
	{
		throw JsonParseExeption("Wrong token for open base64-encoded binary string", pos, is);
	}

	std::string b64;

	while (!is.eof())
	{
		auto c = is.peek();
		if (c == '"')
		{
			throw JsonParseExeption("Wrong token for close base64-encoded binary string", is.tellg(), is);
		}

		if (c == '?')
		{
			is.ignore();

			if (is.peek() != '=')
			{
				throw JsonParseExeption("Wrong token for close base64-encoded binary string", is.tellg(), is);
			}
			is.ignore();

			if (is.peek() != '"')
			{
				throw JsonParseExeption("Wrong token for close base64-encoded binary string", is.tellg(), is);
			}
			is.ignore();

			return std::forward<SBinary>(Base64::decode(b64));
		}

		if (c == -1)
		{
			break;
		}

		b64.push_back(static_cast<uint8_t>(c));
		is.ignore();
	}

	throw JsonParseExeption("Unexpected out of data during parse base64-encoded binary string", is.tellg());
}

SVal JsonSerializer::decodeBool(std::istream& is)
{
	auto pos = is.tellg();
	auto c = is.get();
	if (c == 't')
	{
		if (is.get() == 'r' && is.get() == 'u' && is.get() == 'e')
		{
			return SBool(true);
		}
	}
	else if (c == 'f')
	{
		if (is.get() == 'a' && is.get() == 'l' && is.get() == 's' && is.get() == 'e')
		{
			return SBool(false);
		}
	}

	if (is.eof())
	{
		throw JsonParseExeption("Unexpected end of data during try parse boolean value", is.tellg());
	}

	throw JsonParseExeption("Wrong token for boolean value", pos, is);
}

SVal JsonSerializer::decodeNull(std::istream& is)
{
	auto pos = is.tellg();
	if (
		is.get() == 'n' &&
		is.get() == 'u' &&
		is.get() == 'l' &&
		is.get() == 'l'
	)
	{
		return SNull();
	}

	if (is.eof())
	{
		throw JsonParseExeption("Unexpected end of data during try parse null value", is.tellg());
	}

	throw JsonParseExeption("Wrong token for null value", pos, is);
}

uint32_t JsonSerializer::decodeEscaped(std::istream& is)
{
	if (is.peek() != '\\')
	{
		throw JsonParseExeption("Wrong token for start escaped-sequence", is.tellg(), is);
	}
	is.ignore();

	if (is.eof())
	{
		throw JsonParseExeption("Unexpected end of data during try parse escaped symbol", is.tellg());
	}

	auto c = is.get();
	switch (c)
	{
		case '"':
			return '"';
		case '\\':
			return '\\';
		case '/':
			return '/';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 't':
			return '\t';
		case 'r':
			return '\r';
		case 'u':
			break;
		default:
			is.seekg(-1, std::iostream::cur);
			throw JsonParseExeption("Wrong token for escaped-sequece", is.tellg(), is);
	}

	uint32_t val = 0;
	for (int i = 0; i < 4; i++)
	{
		if (is.eof())
		{
			throw JsonParseExeption("Unxpected end of data during parse escaped-sequence of utf8 symbol", is.tellg());
		}
		c = is.get();
		if (c >= '0' && c <= '9')
		{
			val = (val << 4) | ((c - '0') & 0x0F);
		}
		else if (c >= 'A' && c <= 'F')
		{
			val = (val << 4) | ((10 + c - 'A') & 0x0F);
		}
		else if (c >= 'a' && c <= 'f')
		{
			val = (val << 4) | ((10 + c - 'a') & 0x0F);
		}
		else
		{
			is.seekg(-1, std::iostream::cur);
			throw JsonParseExeption("Wrong token for escaped-sequece of utf8 symbol", is.tellg(), is);
		}
	}

	return val;
}

SVal JsonSerializer::decodeNumber(std::istream& is)
{
	auto pos = is.tellg();

	// Пропускаем ведущий минус
	if (is.peek() == '-')
	{
		is.ignore();
	}

	// Наличие цифр в начале
	if (!std::isdigit(is.peek()))
	{
		throw JsonParseExeption("Wrong token for numeric value", is.tellg(), is);
	}

	while (std::isdigit(is.peek()))
	{
		is.ignore();
	}
	auto c = is.peek();
	if (c == '.' || c == 'e' || c == 'E')
	{
		if (c == '.')
		{
			is.ignore();
			if (!std::isdigit(is.peek()))
			{
				throw JsonParseExeption("Wrong token for numeric value", is.tellg(), is);
			}
			while (std::isdigit(is.peek()))
			{
				is.ignore();
			}
			c = is.peek();
		}
		if (c == 'e' || c == 'E')
		{
			is.ignore();
			c = is.peek();
			if (c == '-' || c == '+')
			{
				is.ignore();
			}
			if (!std::isdigit(is.peek()))
			{
				throw JsonParseExeption("Wrong token for numeric value", is.tellg(), is);
			}
			while (std::isdigit(is.peek()))
			{
				is.ignore();
			}
			c = is.peek();
		}
		if (c == ',' || c == ']' || c == '}' || std::isspace(c) || is.eof())
		{
			is.clear(std::istream::goodbit);
			is.seekg(pos);

			SFloat::type value = 0;
			is >> value;

			return SFloat(value);
		}
	}
	else if (c == ',' || c == ']' || c == '}' || std::isspace(c) || is.eof())
	{
		is.clear(std::istream::goodbit);
		is.seekg(pos);

		SInt::type value;
		is >> value;

		return SInt(value);
	}

	throw JsonParseExeption("Wrong token for numeric value", is.tellg(), is);
}

void JsonSerializer::encodeNull(std::ostream &os, const SVal&)
{
	os << "null";
}

void JsonSerializer::encodeBool(std::ostream &os, const SVal& value)
{
	os << (value.as<SBool>().value() ? "true" : "false");
}

void JsonSerializer::encodeString(std::ostream &os, const SVal& value)
{
	const auto& string = value.as<std::string>();

	os.put('"');
	for (size_t i = 0; i < string.length(); i++)
	{
		auto c = static_cast<uint8_t>(string[i]);
		switch (c)
		{
			case '"':
				os.put('\\');
				os.put('"');
				break;
			case '\\':
				os.put('\\');
				os.put('\\');
				break;
			case '/':
				if ((_flags & ESCAPED_UNICODE) != 0)
				{
					os.put('\\');
				}
				os.put('/');
				break;
			case '\b':
				os.put('\\');
				os.put('b');
				break;
			case '\f':
				os.put('\\');
				os.put('f');
				break;
			case '\n':
				os.put('\\');
				os.put('n');
				break;
			case '\t':
				os.put('\\');
				os.put('t');
				break;
			case '\r':
				os.put('\\');
				os.put('r');
				break;
			default:
				if ((_flags & ESCAPED_UNICODE) == 0)
				{
					os.put(c);
					break;
				}
				// Некорректный символ
				if (c > 0b1111'1101)
				{
					throw std::runtime_error("Bad symbol in string value");
				}
				// Однобайтовый символ
				else if (c < 0b1000'0000)
				{
					os.put(c);
				}
				else
				{
					int bytes;
					uint32_t chr = 0;
					if ((c & 0b11111100) == 0b11111100)
					{
						bytes = 6;
						chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1);
					}
					else if ((c & 0b11111000) == 0b11111000)
					{
						bytes = 5;
						chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11);
					}
					else if ((c & 0b11110000) == 0b11110000)
					{
						bytes = 4;
						chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b111);
					}
					else if ((c & 0b11100000) == 0b11100000)
					{
						bytes = 3;
						chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1111);
					}
					else if ((c & 0b11000000) == 0b11000000)
					{
						bytes = 2;
						chr = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11111);
					}
					else
					{
						throw std::runtime_error("Bad symbol in string value");
					}
					while (--bytes > 0)
					{
						if (++i >= string.length())
						{
							throw std::runtime_error("Uncompleted utf8 symbol");
						}
						c = static_cast<uint8_t>(string[i]);
						if ((c & 0b11000000) != 0b10000000)
						{
							throw std::runtime_error("Bad symbol in string value");
						}
						chr = (chr << 6) | (c & 0b0011'1111);
					}
					os << "\\u" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << chr << std::dec;
				}
		}
	}
	os.put('"');
}

void JsonSerializer::encodeBinary(std::ostream &os, const SVal& value)
{
//	os << "\"=?B?" << Base64::encode(value.value().data(), value.value().size()) << "?=\"";
	const auto& binary = value.as<SBinary>();
	os << "\"=?B?" << Base64::encode(binary.data(), binary.size()) << "?=\"";
}

void JsonSerializer::encodeNumber(std::ostream &os, const SVal& value)
{
	if (value.is<SInt>())
	{
		os << value.as<SInt>().value();
	}
	else if (value.is<SFloat>())
	{
		os << std::setprecision(std::numeric_limits<SFloat::type>::digits10+1) << value.as<SFloat>().value();
	}
}

void JsonSerializer::encodeArray(std::ostream &os, const SVal& value)
{
	const auto& array = value.as<SArr>();

	os << '[';

	if (!array.empty())
	{
		std::for_each(array.begin(), array.end(),
			[this, &os]
			(auto& element)
			{
				this->encodeValue(os, element);
				os << ',';
			}
		);
		os.seekp(-1, std::ios_base::cur);
	}

	os << ']';
}

void JsonSerializer::encodeObject(std::ostream &os, const SVal& value)
{
	const auto& object = value.as<SObj>();

	os << '{';

	if (!object.empty())
	{
		std::for_each(object.begin(), object.end(),
			[this, &os]
			(auto& element)
			{
				this->encodeString(os, element.first);
				os << ':';
				this->encodeValue(os, element.second);
				os << ',';
			}
		);
		os.seekp(-1, std::ios_base::cur);
	}

	os << '}';
}

void JsonSerializer::encodeValue(std::ostream &os, const SVal& value)
{
	if (value.is<SStr>())
	{
		encodeString(os, value);
	}
	else if (value.is<SInt>())
	{
		encodeNumber(os, value);
	}
	else if (value.is<SFloat>())
	{
		encodeNumber(os, value);
	}
	else if (value.is<SObj>())
	{
		encodeObject(os, value);
	}
	else if (value.is<SArr>())
	{
		encodeArray(os, value);
	}
	else if (value.is<SBool>())
	{
		encodeBool(os, value);
	}
	else if (value.is<SNull>())
	{
		encodeNull(os, value);
	}
	else if (value.is<SBinary>())
	{
		encodeBinary(os, value);
	}
	else if (value.isUndefined())
	{
		throw std::runtime_error("Undefined value");
	}
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}

void JsonSerializer::putUnicodeSymbolAsUtf8(SStr& str, uint32_t symbol)
{
	if (symbol <= 0b0111'1111) // 7bit -> 1byte
	{
		str.push_back(symbol);
	}
	else if (symbol <= 0b0111'1111'1111) // 11bit -> 2byte
	{
		str.push_back(0b1100'0000 | (0b0001'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b1111'1111'1111'1111) // 16bit -> 3byte
	{
		str.push_back(0b1110'0000 | (0b0000'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0001'1111'1111'1111'1111'1111) // 21bit -> 4byte
	{
		str.push_back(0b1111'0000 | (0b0000'0111 & (symbol >> 18)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0011'1111'1111'1111'1111'1111'1111) // 26bit -> 5byte
	{
		str.push_back(0b1111'1000 | (0b0000'0011 & (symbol >> 24)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0111'1111'1111'1111'1111'1111'1111'1111) // 31bit -> 6byte
	{
		str.push_back(0b1111'1100 | (0b0000'0001 & (symbol >> 30)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 24)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
}
