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
// File created on: 2017.05.03

// TlvSerializer.cpp


#include "TlvSerializer.hpp"

#include <cstring>

#ifdef NAN
#undef NAN
#endif

#ifdef NULL
#undef NULL
#endif

enum class Token {
	END 		= 0,
	FALSE		= 1,
	TRUE		= 2,
	NULL		= 3,
	NAN			= 4,
	ZERO		= 10,
	NEG_INT_8	= 11,
	POS_INT_8	= 12,
	NEG_INT_16	= 13,
	POS_INT_16	= 14,
	NEG_INT_32	= 15,
	POS_INT_32	= 16,
	NEG_INT_64	= 17,
	POS_INT_64	= 18,
//	FLOAT_8		= 21,
//	FLOAT_16	= 22,
	FLOAT_32	= 23,
	FLOAT_64	= 24,
	STRING		= 30,
	ARRAY		= 40,
	ARRAY_END	= static_cast<int>(Token::END),
	OBJECT		= 50,
	OBJECT_END	= static_cast<int>(Token::END),
//	RECURSION	= 60,
//	UNAVAILABLE	= 70,
	BINARY_255	= 81,
	BINARY_64K	= 82,
	BINARY_4G	= 83,
//	_KEY		= 200,
//	_EXTERNAL	= 250,
	UNKNOWN		= 255
};

SVal TlvSerializer::decode(const std::string& data)
{
	_iss.str(data);
	_iss.clear(_iss.goodbit);

	if (_iss.eof() || _iss.peek() == -1)
	{
		throw std::runtime_error("No data for parsing");
	}

	SVal value;

	try
	{
		value = decodeValue();

		// Проверяем лишние данные в конце
		if (!_iss.eof() && (_flags & Serializer::STRICT) != 0)
		{
			throw std::runtime_error("Redundant bytes after parsed data");
		}
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't decode TLV ← ") + exception.what());
	}

	return value;
}

std::string TlvSerializer::encode(const SVal& value)
{
	try
	{
		encodeValue(value);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't encode into TLV ← ") + exception.what());
	}

	return std::move(_oss.str());
}

SVal TlvSerializer::decodeValue()
{
	if (_iss.eof())
	{
		throw std::runtime_error("Unexpect out of data during parse value");
	}

	auto token = static_cast<Token>(_iss.peek());
	switch (token)
	{
		case Token::FALSE:
		case Token::TRUE: return decodeBool();

		case Token::NULL: return decodeNull();

		case Token::ZERO:
		case Token::NEG_INT_8:
		case Token::POS_INT_8:
		case Token::NEG_INT_16:
		case Token::POS_INT_16:
		case Token::NEG_INT_32:
		case Token::POS_INT_32:
		case Token::NEG_INT_64:
		case Token::POS_INT_64: return decodeInteger();

//		case Token::FLOAT_8:
//		case Token::FLOAT_16:
		case Token::FLOAT_32:
		case Token::FLOAT_64: return decodeFloat();

		case Token::STRING: return decodeString();

		case Token::ARRAY: return decodeArray();

		case Token::OBJECT: return decodeObject();

//		case Token::NAN;
//			return decodeNan();
//
//		case Token::END:
//		case Token::RECURSION:
//		case Token::UNAVAILABLE:
		case Token::BINARY_255:
		case Token::BINARY_64K:
		case Token::BINARY_4G: return decodeBinary();

//		case Token::_KEY:
//		case Token::_EXTERNAL:
		case Token::UNKNOWN:
		default: throw std::runtime_error("Unknown token at parse value");
	}
}

SVal TlvSerializer::decodeObject()
{
	SObj obj;

	auto c = static_cast<Token>(_iss.get());
	if (c != Token::OBJECT)
	{
		throw std::runtime_error("Wrong token for open object");
	}

	if (static_cast<Token>(_iss.peek()) == Token::OBJECT_END)
	{
		_iss.ignore();
		return obj;
	}

	while (!_iss.eof())
	{
		SStr key;
		try
		{
			key = decodeString().as<SStr>();
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse field-key of object ← ") + exception.what());
		}

		try
		{
			obj.emplace(key.value(), decodeValue());
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse field-value of object ← ") + exception.what());
		}

		if (static_cast<Token>(_iss.peek()) == Token::OBJECT_END)
		{
			_iss.ignore();
			return obj;
		}
	}

	throw std::runtime_error("Unexpect out of data during parse object");
}

SVal TlvSerializer::decodeArray()
{
	SArr arr;

	auto c = static_cast<Token>(_iss.get());
	if (c != Token::ARRAY)
	{
		throw std::runtime_error("Wrong token for open array");
	}

	if (static_cast<Token>(_iss.peek()) == Token::ARRAY_END)
	{
		_iss.ignore();
		return std::move(arr);
	}

	while (!_iss.eof())
	{
		try
		{
			arr.emplace_back(decodeValue());
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse element of array ← ") + exception.what());
		}

		if (static_cast<Token>(_iss.peek()) == Token::ARRAY_END)
		{
			_iss.ignore();
			return std::move(arr);
		}
	}

	throw std::runtime_error("Unexpect out of data during parse array");
}

SVal TlvSerializer::decodeString()
{
	SStr str;

	if (static_cast<Token>(_iss.get()) != Token::STRING)
	{
		throw std::runtime_error("Wrong token for open string");
	}

	while (_iss.peek() != -1)
	{
		auto c = _iss.peek();
		// Конец строки
		if (static_cast<Token>(c) == Token::END)
		{
			_iss.ignore();
			return std::move(str);
		}

		// Управляющий символ
		if (c < ' ' && c != '\t' && c != '\r' && c != '\n')
		{
			throw std::runtime_error("Bad symbol in string value");
		}

		// Некорректный символ
		if (c > 0b1111'1101)
		{
			throw std::runtime_error("Bad symbol in string value");
		}

		// Однобайтовый символ
		if (c < 0b1000'0000)
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

SVal TlvSerializer::decodeBinary()
{
	auto token = static_cast<Token>(_iss.get());

	size_t size;

	if (token == Token::BINARY_255)
	{
		size = static_cast<uint8_t>(_iss.get());
	}
	else if (token == Token::BINARY_64K)
	{
		uint16_t size_le;
		_iss.read(reinterpret_cast<char*>(&size_le), sizeof(size_le));
		size = le16toh(size_le);
	}
	else if (token == Token::BINARY_4G)
	{
		uint32_t size_le;
		_iss.read(reinterpret_cast<char*>(&size_le), sizeof(size_le));
		size = le32toh(size_le);
	}
	else
	{
		throw std::runtime_error("Wrong token for open binary");
	}

	std::string bin;

	while (size-- > 0)
	{
		if (!_iss.eof() && _iss.peek() == -1)
		{
			throw std::runtime_error("Unexpect out of data during parse binary data");
		}
		bin.push_back(static_cast<std::string::value_type>(_iss.get()));
	}

	return new SBinary(bin);
}

SVal TlvSerializer::decodeBool()
{
	auto c = static_cast<Token>(_iss.get());
	switch (c)
	{
		case Token::TRUE: return new SBool(true);

		case Token::FALSE: return new SBool(false);

		default: throw std::runtime_error("Wrong token for boolean value");
	}
}

SVal TlvSerializer::decodeNull()
{
	if (static_cast<Token>(_iss.get()) != Token::NULL)
	{
		throw std::runtime_error("Wrong token for null value");
	}

	return new SNull();
}

SVal TlvSerializer::decodeInteger()
{
	int bytes = 0;
	bool negative = false;

	auto token = static_cast<Token>(_iss.get());
	switch (token)
	{
		case Token::NEG_INT_64: negative = true;
		case Token::POS_INT_64: bytes = 8;
			break;
		case Token::NEG_INT_32: negative = true;
		case Token::POS_INT_32: bytes = 4;
			break;
		case Token::NEG_INT_16: negative = true;
		case Token::POS_INT_16: bytes = 2;
			break;
		case Token::NEG_INT_8: negative = true;
		case Token::POS_INT_8: bytes = 1;
			break;
		case Token::ZERO: bytes = 0;
			break;
		default: throw std::runtime_error("Wrong token for integer value");
	}

	union
	{
		uint64_t i;
		char c[8];
	} data = {0x00};

	for (int i = 0; i < bytes; i++)
	{
		auto c = _iss.get();
		if (c == -1)
		{
			throw std::runtime_error("Unexpect out of data during parse number value");
		}
		data.c[i] = static_cast<char>(c);
	}

	return new SInt((negative ? -1 : 1) * le64toh(data.i));
}

SVal TlvSerializer::decodeFloat()
{
	auto token = static_cast<Token>(_iss.get());
	switch (token)
	{
		case Token::FLOAT_32:
		{
			union
			{
				float_t f;
				char c[4];
			} data = {0};

			for (char& i : data.c)
			{
				auto c = _iss.get();
				if (c == -1)
				{
					throw std::runtime_error("Unexpect out of data during parse number value");
				}
				i = static_cast<char>(c);
			}

			return new SFloat(le32toh(data.f));
		}
		case Token::FLOAT_64:
		{
			union
			{
				double_t f;
				char c[4];
			} data = {0};

			for (int i = 0; i < 8; i++)
			{
				auto c = _iss.get();
				if (c == -1)
				{
					throw std::runtime_error("Unexpect out of data during parse number value");
				}
				data.c[i] = static_cast<char>(c);
			}

			return new SFloat(le64toh(data.f));
		}
		default: throw std::runtime_error("Wrong token for float value");
	}
}

void TlvSerializer::encodeNull(const SVal& value)
{
	_oss.put(static_cast<char>(Token::NULL));
}

void TlvSerializer::encodeBool(const SVal& value)
{
	_oss.put(static_cast<char>(value.as<SBool>() ? Token::TRUE : Token::FALSE));
}

void TlvSerializer::encodeString(const SVal& value)
{
	_oss.put(static_cast<char>(Token::STRING));
	_oss << value.as<SStr>().value();
	_oss.put(static_cast<char>(Token::END));
}

void TlvSerializer::encodeKey(const std::string& key)
{
	_oss.put(static_cast<char>(Token::STRING));
	_oss << key;
	_oss.put(static_cast<char>(Token::END));
}

void TlvSerializer::encodeBinary(const SVal& value)
{
	const auto& bin = value.as<SBinary>();

	if (bin.value().size() <= std::numeric_limits<uint8_t>::max())
	{
		_oss.put(static_cast<char>(Token::BINARY_255));
		_oss.put(static_cast<uint8_t>(bin.value().size()));
	}
	else if (bin.value().size() <= std::numeric_limits<uint16_t>::max())
	{
		_oss.put(static_cast<char>(Token::BINARY_64K));
		auto size_le = htole16(static_cast<uint16_t>(bin.value().size()));
		for (size_t i = 0; i < sizeof(size_le); i++)
		{
			_oss.put(reinterpret_cast<uint8_t*>(&size_le)[i]);
		}
	}
	else if (bin.value().size() <= std::numeric_limits<uint32_t>::max())
	{
		_oss.put(static_cast<char>(Token::BINARY_4G));
		auto size_le = htole32(static_cast<uint32_t>(bin.value().size()));
		for (size_t i = 0; i < sizeof(size_le); i++)
		{
			_oss.put(reinterpret_cast<uint8_t*>(&size_le)[i]);
		}
	}
	else
	{
		throw std::runtime_error("Too long binary data");
	}

	_oss.write(bin.value().data(), bin.value().size());
}

void TlvSerializer::encodeNumber(const SVal& value)
{
	if (value.is<SInt>())
	{
		if (value.as<SInt>().value() == 0)
		{
			_oss.put(static_cast<char>(Token::ZERO));
			return;
		}

		int bytes = 0;

		bool negative = value.as<SInt>().value() < 0;

		auto absValue = static_cast<uint64_t>(llabs(value.as<SInt>().value()));

		if (absValue == 0)
		{
			_oss.put(static_cast<char>(Token::ZERO));
			bytes = 0;
		}
		else if (absValue <= UINT8_MAX)
		{
			_oss.put(static_cast<char>(negative ? Token::NEG_INT_8 : Token::POS_INT_8));
			bytes = 1;
		}
		else if (absValue <= UINT16_MAX)
		{
			_oss.put(static_cast<char>(negative ? Token::NEG_INT_16 : Token::POS_INT_16));
			bytes = 2;
		}
		else if (absValue <= UINT32_MAX)
		{
			_oss.put(static_cast<char>(negative ? Token::NEG_INT_32 : Token::POS_INT_32));
			bytes = 4;
		}
		else if (absValue <= UINT64_MAX)
		{
			_oss.put(static_cast<char>(negative ? Token::NEG_INT_64 : Token::POS_INT_64));
			bytes = 8;
		}
		else
		{
			throw std::runtime_error("Bad numeric value");
		}

		union
		{
			uint64_t i;
			char c[8];
		} data = {htole64(absValue)};

		for (auto i = 0; i < bytes; i++)
		{
			_oss.put(data.c[i]);
		}
		return;
	}
	if (value.is<SFloat>())
	{
		auto i64 = static_cast<int64_t>(value.as<SFloat>().value());
		if (static_cast<decltype(value.as<SFloat>().value())>(i64) == value.as<SFloat>().value())
		{
			encodeNumber(i64);
			return;
		}

		auto f32 = static_cast<float_t>(value.as<SFloat>().value());
		if (static_cast<decltype(value.as<SFloat>().value())>(f32) == value.as<SFloat>().value())
		{
			char data[4];

			f32 = htole32(f32);
			memcpy(data, &f32, sizeof(data));

			_oss.put(static_cast<char>(Token::FLOAT_32));
			for (char i : data)
			{
				_oss.put(i);
			}
			return;
		}

		auto f64 = static_cast<double_t>(value.as<SFloat>().value());

		char data[8];

		f64 = htole64(f64);
		memcpy(data, &f64, sizeof(data));

		_oss.put(static_cast<char>(Token::FLOAT_64));
		for (char i : data)
		{
			_oss.put(i);
		}
	}
}

void TlvSerializer::encodeArray(const SVal& value)
{
	const auto& array = value.as<SArr>();

	_oss.put(static_cast<char>(Token::ARRAY));
	std::for_each(array.cbegin(), array.cend(),
		[this]
		(const SVal& val)
		{
			encodeValue(val);
		}
	);
	_oss.put(static_cast<char>(Token::ARRAY_END));
}

void TlvSerializer::encodeObject(const SVal& value)
{
	const auto& object = value.as<SObj>();

	_oss.put(static_cast<char>(Token::OBJECT));
	std::for_each(object.cbegin(), object.cend(),
		[this]
		(const std::pair<std::string, SVal>& element)
		{
			encodeKey(element.first);
			encodeValue(element.second);
		}
	);
	_oss.put(static_cast<char>(Token::OBJECT_END));
}

void TlvSerializer::encodeValue(const SVal& value)
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

void TlvSerializer::putUtf8Symbol(SStr& str, uint32_t symbol)
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
