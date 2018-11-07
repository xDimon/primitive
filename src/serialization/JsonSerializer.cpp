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
// File created on: 2017.04.07

// JsonSerializer.cpp


#include "JsonSerializer.hpp"
#include "../utils/encoding/Base64.hpp"

#include <iomanip>

REGISTER_SERIALIZER(json, JsonSerializer);

SVal JsonSerializer::decode(const std::string& data)
{
	_iss.str(data);
	_iss.clear();

	if (_iss.eof() || _iss.peek() == -1)
	{
		throw std::runtime_error("No data for parsing");
	}

	SVal value;

	try
	{
		value = decodeValue();
	}
	catch (std::exception& exception)
	{
		throw std::runtime_error(std::string("Can't decode JSON ← ") + exception.what());
	}

	// Проверяем лишние символы в конце
	skipSpaces();
	if (!_iss.eof() && (_flags & Serializer::STRICT) != 0)
	{
		throw std::runtime_error("Redundant bytes after parsed data");
	}

	return value;
}

std::string JsonSerializer::encode(const SVal& value)
{
	_oss.str("");
	_oss.clear();

	try
	{
		encodeValue(value);
	}
	catch (std::exception& exception)
	{
		throw std::runtime_error(std::string("Can't encode into JSON ← ") + exception.what());
	}

	return std::move(_oss.str());
}

void JsonSerializer::skipSpaces()
{
	while (!_iss.eof())
	{
		auto c = _iss.peek();
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			return;
		}
		_iss.ignore();
	}
}

SVal JsonSerializer::decodeValue()
{
	skipSpaces();

	if (_iss.eof())
	{
		throw std::runtime_error("Unexpect out of data during parse value");
	}

	switch (_iss.peek())
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
		{
			return decodeNumber();
		}

		case '"':
		{
			auto p = _iss.tellg();
			if (_iss.get() == '"')
			{
				if (_iss.get() == '=')
				{
					if (_iss.get() == '?')
					{
						if (_iss.get() == 'B')
						{
							if (_iss.get() == '?')
							{
								_iss.clear(_iss.goodbit);
								_iss.seekg(p);
								return decodeBinary();
							}
						}
					}
				}
			}
			_iss.clear(_iss.goodbit);
			_iss.seekg(p);
			return decodeString();
		}

		case 't':
		case 'f':
		{
			return decodeBool();
		}

		case '{':
		{
			return decodeObject();
		}

		case '[':
		{
			return decodeArray();
		}

		case 'n':
		{
			return decodeNull();
		}

		default:
			throw std::runtime_error("Unknown token at parse value");
	}
}

SVal JsonSerializer::decodeObject()
{
	auto c = _iss.get();
	if (c != '{')
	{
		throw std::runtime_error("Wrong token for open object");
	}

	skipSpaces();

	SObj obj;

	if (_iss.peek() == '}')
	{
		_iss.ignore();
		return std::move(obj);
	}

	while (!_iss.eof())
	{
		std::string key;
		try
		{
			key = std::move(decodeString().as<SStr>().value());
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse field-key of object ← ") + exception.what());
		}

		skipSpaces();

		c = _iss.get();
		if (c != ':')
		{
			throw std::runtime_error("Wrong token after field-key of object");
		}

		skipSpaces();

		try
		{
			obj.emplace(std::move(key), std::move(decodeValue()));
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse field-value of object ← ") + exception.what());
		}

		skipSpaces();

		c = _iss.get();
		if (c == ',')
		{
			skipSpaces();
			continue;
		}

		if (c == '}')
		{
			return std::move(obj);
		}

		throw std::runtime_error("Wrong token after field-value of object");
	}

	throw std::runtime_error("Unexpect out of data during parse object");
}

SVal JsonSerializer::decodeArray()
{
	auto c = _iss.get();
	if (c != '[')
	{
		throw std::runtime_error("Wrong token for open array");
	}

	skipSpaces();

	SArr arr;

	if (_iss.peek() == ']')
	{
		_iss.ignore();
		return std::move(arr);
	}

	while (!_iss.eof())
	{
		try
		{
			arr.emplace_back(std::move(decodeValue()));
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse element of array ← ") + exception.what());
		}

		skipSpaces();

		c = _iss.get();
		if (c == ',')
		{
			skipSpaces();
			continue;
		}

		if (c == ']')
		{
			return std::move(arr);
		}

		throw std::runtime_error("Wrong token after element of array");
	}

	throw std::runtime_error("Unexpect out of data during parse array");
}

SVal JsonSerializer::decodeString()
{
	auto c = _iss.get();
	if (c != '"')
	{
		throw std::runtime_error("Wrong token for open string");
	}

	SStr str;

	while (_iss.peek() != -1)
	{
		c = _iss.peek();
		// Конец строки
		if (c == '"')
		{
			_iss.ignore();
			return std::move(str);
		}

		// Экранированный сивол
		if (c == '\\')
		{
			uint32_t symbol = decodeEscaped();

			if (symbol >= 0xD800 && symbol <= 0xDFFF) // Суррогатная пара UTF16
			{
				if ((symbol & 0b1111'1100'0000'0000) != 0b1101'1000'0000'0000)
				{
					throw std::runtime_error("Bad escaped symbol: wrong for first byte of utf-16 surrogate pair");
				}
				uint32_t symbol2 = decodeEscaped();
				if ((symbol2 & 0b1111'1100'0000'0000) != 0b1101'1100'0000'0000)
				{
					throw std::runtime_error("Bad escaped symbol: wrong for second byte of utf-16 surrogate pair");
				}
				symbol = ((((symbol >> 6) & 0b1111) + 1) << 16) | ((symbol & 0b11'1111) << 10) |
						 (symbol2 & 0b0011'1111'1111);
			}

			putUtf8Symbol(str, symbol);
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
			_iss.ignore();
			str.insert(static_cast<uint8_t>(c));
		}
		else
		{
			_iss.ignore();
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
				if (_iss.eof())
				{
					throw std::runtime_error("Unxpected end of data during parse utf8 symbol");
				}
				c = _iss.get();
				if ((c & 0b11000000) != 0b10000000)
				{
					throw std::runtime_error("Bad symbol in string value");
				}
				chr = (chr << 6) | (c & 0b0011'1111);
			}
			putUtf8Symbol(str, chr);
		}
	}

	throw std::runtime_error("Unexpect out of data during parse string value");
}

SVal JsonSerializer::decodeBinary()
{
	bool ok = false;

	if (_iss.get() == '"')
	{
		if (_iss.get() == '=')
		{
			if (_iss.get() == '?')
			{
				if (_iss.get() == 'B')
				{
					if (_iss.get() == '?')
					{
						ok = true;
					}
				}
			}
		}
	}

	if (!ok)
	{
		throw std::runtime_error("Wrong token for open base64-encoded binary string");
	}

	std::string b64;

	while (!_iss.eof())
	{
		auto c = _iss.get();
		if (c == '"')
		{
			throw std::runtime_error("Wrong token for close base64-encoded binary string");
		}
		if (c == '?')
		{
			auto c1 = _iss.get();
			if (c1 != '=')
			{
				throw std::runtime_error("Wrong token for close base64-encoded binary string");
			}
			auto c2 = _iss.get();
			if (c2 != '"')
			{
				throw std::runtime_error("Wrong token for close base64-encoded binary string");
			}
			return std::forward<SBinary>(Base64::decode(b64));
		}

		if (c == -1)
		{
			break;
		}
		b64.push_back(static_cast<uint8_t>(c));
	}

	throw std::runtime_error("Unexpect out of data during parse base64-encoded binary string");
}

SVal JsonSerializer::decodeBool()
{
	auto c = _iss.get();
	if (c == 't')
	{
		if (!_iss.eof() && _iss.get() == 'r')
		{
			if (!_iss.eof() && _iss.get() == 'u')
			{
				if (!_iss.eof() && _iss.get() == 'e')
				{
					return true;
				}
			}
		}
	}
	else if (c == 'f')
	{
		if (!_iss.eof() && _iss.get() == 'a')
		{
			if (!_iss.eof() && _iss.get() == 'l')
			{
				if (!_iss.eof() && _iss.get() == 's')
				{
					if (!_iss.eof() && _iss.get() == 'e')
					{
						return false;
					}
				}
			}
		}
	}
	if (_iss.eof())
	{
		throw std::runtime_error("Unxpected end of data during try parse boolean value");
	}

	throw std::runtime_error("Wrong token for boolean value");
}

SVal JsonSerializer::decodeNull()
{
	if (!_iss.eof() && _iss.get() == 'n')
	{
		if (!_iss.eof() && _iss.get() == 'u')
		{
			if (!_iss.eof() && _iss.get() == 'l')
			{
				if (!_iss.eof() && _iss.get() == 'l')
				{
					return nullptr;
				}
			}
		}
	}
	if (_iss.eof())
	{
		throw std::runtime_error("Unxpected end of data during try parse null value");
	}

	throw std::runtime_error("Wrong token for null value");
}

uint32_t JsonSerializer::decodeEscaped()
{
	auto c = _iss.get();
	if (c != '\\')
	{
		throw std::runtime_error("Wrong token for start escaped-sequence");
	}
	if (_iss.eof())
	{
		throw std::runtime_error("Unxpected end of data during try parse escaped symbol");
	}
	c = _iss.get();
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
			throw std::runtime_error("Wrong token for escaped-sequece");
	}
	uint32_t val = 0;
	for (int i = 0; i < 4; i++)
	{
		if (_iss.eof())
		{
			throw std::runtime_error("Unxpected end of data during parse escaped-sequence of utf8 symbol");
		}
		c = _iss.get();
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
			throw std::runtime_error("Wrong token for escaped-sequece of utf8 symbol");
		}
	}

	return val;
}

SVal JsonSerializer::decodeNumber()
{
	auto p = _iss.tellg();

	while (!_iss.eof())
	{
		auto c = _iss.get();
		if (c == '.' || c == 'e' || c == 'E')
		{
			SFloat::type value = 0;

			_iss.clear(_iss.goodbit);
			_iss.seekg(p);
			_iss >> value;

			return value;
		}

		if (isdigit(c) == 0 && c != '-')
		{
			break;
		}
	}

	SInt::type value = 0;

	_iss.clear(_iss.goodbit);
	_iss.seekg(p);
	_iss >> value;

	return value;
}

void JsonSerializer::encodeNull(const SVal&)
{
	_oss << "null";
}

void JsonSerializer::encodeBool(const SVal& value)
{
	_oss << ((bool)value ? "true" : "false");
}

void JsonSerializer::encodeString(const SVal& value)
{
	const auto& string = value.as<std::string>();

	_oss.put('"');
	for (size_t i = 0; i < string.length(); i++)
	{
		auto c = static_cast<uint8_t>(string[i]);
		switch (c)
		{
			case '"':
				_oss.put('\\');
				_oss.put('"');
				break;
			case '\\':
				_oss.put('\\');
				_oss.put('\\');
				break;
			case '/':
				if ((_flags & ESCAPED_UNICODE) != 0)
				{
					_oss.put('\\');
				}
				_oss.put('/');
				break;
			case '\b':
				_oss.put('\\');
				_oss.put('b');
				break;
			case '\f':
				_oss.put('\\');
				_oss.put('f');
				break;
			case '\n':
				_oss.put('\\');
				_oss.put('n');
				break;
			case '\t':
				_oss.put('\\');
				_oss.put('t');
				break;
			case '\r':
				_oss.put('\\');
				_oss.put('r');
				break;
			default:
				if ((_flags & ESCAPED_UNICODE) == 0)
				{
					_oss.put(c);
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
					_oss.put(c);
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
							throw std::runtime_error("Unxpected end of data during parse utf8 symbol");
						}
						c = static_cast<uint8_t>(string[i]);
						if ((c & 0b11000000) != 0b10000000)
						{
							throw std::runtime_error("Bad symbol in string value");
						}
						chr = (chr << 6) | (c & 0b0011'1111);
					}
					_oss << "\\u" << std::setw(4) << std::setfill('0') << std::hex << chr;
				}
		}
	}
	_oss.put('"');
}

void JsonSerializer::encodeBinary(const SVal& value)
{
//	_oss << "\"=?B?" << Base64::encode(value.value().data(), value.value().size()) << "?=\"";
	const auto& binary = value.as<SBinary>();
	_oss << "\"=?B?" << Base64::encode(binary.value().data(), binary.value().size()) << "?=\"";
}

void JsonSerializer::encodeNumber(const SVal& value)
{
	if (value.is<SInt>())
	{
		_oss << (SInt::type)value;
		return;
	}
	if (value.is<SFloat>())
	{
		_oss << std::setprecision(15) << (SFloat::type)value;
		return;
	}
}

void JsonSerializer::encodeArray(const SVal& value)
{
	const auto& array = value.as<SArr>();

	_oss << "[";

	bool empty = true;
	std::for_each(array.begin(), array.end(),
		[this, &empty]
		(const auto& element)
		{
			if (!empty)
			{
				_oss << ",";
			}
			else
			{
				empty = false;
			}
			this->encodeValue(element);
		}
	);

	_oss << "]";
}

void JsonSerializer::encodeObject(const SVal& value)
{
	const auto& object = value.as<SObj>();

	_oss << "{";

	bool empty = true;
	std::for_each(object.begin(), object.end(),
		[this, &empty]
		(const auto& element)
		{
			if (!empty)
			{
				_oss << ",";
			}
			else
			{
				empty = false;
			}
			this->encodeString(element.first);
			_oss << ':';
			this->encodeValue(element.second);
		}
	);

	_oss << "}";
}

void JsonSerializer::encodeValue(const SVal& value)
{
	if (value.is<SStr>())
	{
		encodeString(value);
	}
	else if (value.is<SInt>())
	{
		encodeNumber(value);
	}
	else if (value.is<SFloat>())
	{
		encodeNumber(value);
	}
	else if (value.is<SObj>())
	{
		encodeObject(value);
	}
	else if (value.is<SArr>())
	{
		encodeArray(value);
	}
	else if (value.is<SBool>())
	{
		encodeBool(value);
	}
	else if (value.is<SNull>())
	{
		encodeNull(value);
	}
	else if (value.is<SBinary>())
	{
		encodeBinary(value);
	}
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}

void JsonSerializer::putUtf8Symbol(SStr& str, uint32_t symbol)
{
	if (symbol <= 0b0111'1111) // 7bit -> 1byte
	{
		str.insert(symbol);
	}
	else if (symbol <= 0b0111'1111'1111) // 11bit -> 2byte
	{
		str.insert(0b1100'0000 | (0b0001'1111 & (symbol >> 6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b1111'1111'1111'1111) // 16bit -> 3byte
	{
		str.insert(0b1110'0000 | (0b0000'1111 & (symbol >> 12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0001'1111'1111'1111'1111'1111) // 21bit -> 4byte
	{
		str.insert(0b1111'0000 | (0b0000'0111 & (symbol >> 18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0011'1111'1111'1111'1111'1111'1111) // 26bit -> 5byte
	{
		str.insert(0b1111'1000 | (0b0000'0011 & (symbol >> 24)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
	else if (symbol <= 0b0111'1111'1111'1111'1111'1111'1111'1111) // 31bit -> 6byte
	{
		str.insert(0b1111'1100 | (0b0000'0001 & (symbol >> 30)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 24)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.insert(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
	}
}
