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
// File created on: 2017.05.03

// TlvSerializer.cpp


#include "TlvSerializer.hpp"

#include <cstring>
//#include "SBool.hpp"
#include "SObj.hpp"
#include "SStr.hpp"
//#include "SVal.hpp"
#include "SArr.hpp"
//#include "SNull.hpp"
//#include "SNum.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SBinary.hpp"

REGISTER_SERIALIZER(tlv, TlvSerializer);

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

SVal TlvSerializer::decode(std::istream& is)
{
	if (is.eof() || is.peek() == -1)
	{
		throw std::runtime_error("No data for parsing");
	}

	SVal value;

	try
	{
		value = decodeValue(is);

		// Проверяем лишние данные в конце
		if (!is.eof() && (_flags & Serializer::STRICT) != 0)
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

void TlvSerializer::encode(std::ostream& os, const SVal& value)
{
	try
	{
		encodeValue(os, value);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't encode into TLV ← ") + exception.what());
	}
}

SVal TlvSerializer::decodeValue(std::istream& is)
{
	if (is.eof())
	{
		throw std::runtime_error("Unexpect out of data during parse value");
	}

	auto token = static_cast<Token>(is.peek());
	switch (token)
	{
		case Token::FALSE:
		case Token::TRUE: return decodeBool(is);

		case Token::NULL: return decodeNull(is);

		case Token::ZERO:
		case Token::NEG_INT_8:
		case Token::POS_INT_8:
		case Token::NEG_INT_16:
		case Token::POS_INT_16:
		case Token::NEG_INT_32:
		case Token::POS_INT_32:
		case Token::NEG_INT_64:
		case Token::POS_INT_64: return decodeInteger(is);

//		case Token::FLOAT_8:
//		case Token::FLOAT_16:
		case Token::FLOAT_32:
		case Token::FLOAT_64: return decodeFloat(is);

		case Token::STRING: return decodeString(is);

		case Token::ARRAY: return decodeArray(is);

		case Token::OBJECT: return decodeObject(is);

//		case Token::NAN;
//			return decodeNan(_is);
//
//		case Token::END:
//		case Token::RECURSION:
//		case Token::UNAVAILABLE:
		case Token::BINARY_255:
		case Token::BINARY_64K:
		case Token::BINARY_4G: return decodeBinary(is);

//		case Token::_KEY:
//		case Token::_EXTERNAL:
		case Token::UNKNOWN:
		default: throw std::runtime_error("Unknown token at parse value");
	}
}

SVal TlvSerializer::decodeObject(std::istream& is)
{
	SObj obj;

	auto c = static_cast<Token>(is.get());
	if (c != Token::OBJECT)
	{
		throw std::runtime_error("Wrong token for open object");
	}

	if (static_cast<Token>(is.peek()) == Token::OBJECT_END)
	{
		is.ignore();
		return obj;
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
			throw std::runtime_error(std::string("Can't parse field-key of object ← ") + exception.what());
		}

		try
		{
			obj.emplace(key, decodeValue(is));
		}
		catch (const std::runtime_error& exception)
		{
			throw std::runtime_error(std::string("Can't parse field-value of object ← ") + exception.what());
		}

		if (static_cast<Token>(is.peek()) == Token::OBJECT_END)
		{
			is.ignore();
			return obj;
		}
	}

	throw std::runtime_error("Unexpect out of data during parse object");
}

SVal TlvSerializer::decodeArray(std::istream& is)
{
	SArr arr;

	auto c = static_cast<Token>(is.get());
	if (c != Token::ARRAY)
	{
		throw std::runtime_error("Wrong token for open array");
	}

	if (static_cast<Token>(is.peek()) == Token::ARRAY_END)
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
			throw std::runtime_error(std::string("Can't parse element of array ← ") + exception.what());
		}

		if (static_cast<Token>(is.peek()) == Token::ARRAY_END)
		{
			is.ignore();
			return std::move(arr);
		}
	}

	throw std::runtime_error("Unexpect out of data during parse array");
}

SVal TlvSerializer::decodeString(std::istream& is)
{
	SStr str;

	if (static_cast<Token>(is.get()) != Token::STRING)
	{
		throw std::runtime_error("Wrong token for open string");
	}

	while (is.peek() != -1)
	{
		auto c = is.peek();
		// Конец строки
		if (static_cast<Token>(c) == Token::END)
		{
			is.ignore();
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
				throw std::runtime_error("Bad symbol in string value");
			}
			while (--bytes > 0)
			{
				if (is.eof())
				{
					throw std::runtime_error("Unxpected end of data during parse utf8 symbol");
				}
				c = is.get();
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

SVal TlvSerializer::decodeBinary(std::istream& is)
{
	auto token = static_cast<Token>(is.get());

	size_t size;

	if (token == Token::BINARY_255)
	{
		size = static_cast<uint8_t>(is.get());
	}
	else if (token == Token::BINARY_64K)
	{
		uint16_t size_le;
		is.read(reinterpret_cast<char*>(&size_le), sizeof(size_le));
		size = le16toh(size_le);
	}
	else if (token == Token::BINARY_4G)
	{
		uint32_t size_le;
		is.read(reinterpret_cast<char*>(&size_le), sizeof(size_le));
		size = le32toh(size_le);
	}
	else
	{
		throw std::runtime_error("Wrong token for open binary");
	}

	std::string bin;

	while (size-- > 0)
	{
		if (!is.eof() && is.peek() == -1)
		{
			throw std::runtime_error("Unexpect out of data during parse binary data");
		}
		bin.push_back(static_cast<std::string::value_type>(is.get()));
	}

	return SBinary(bin);
}

SVal TlvSerializer::decodeBool(std::istream& is)
{
	auto c = static_cast<Token>(is.get());
	switch (c)
	{
		case Token::TRUE: return SBool(true);

		case Token::FALSE: return SBool(false);

		default: throw std::runtime_error("Wrong token for boolean value");
	}
}

SVal TlvSerializer::decodeNull(std::istream& is)
{
	if (static_cast<Token>(is.get()) != Token::NULL)
	{
		throw std::runtime_error("Wrong token for null value");
	}

	return SNull();
}

SVal TlvSerializer::decodeInteger(std::istream& is)
{
	int bytes = 0;
	bool negative = false;

	auto token = static_cast<Token>(is.get());
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
		auto c = is.get();
		if (c == -1)
		{
			throw std::runtime_error("Unexpect out of data during parse number value");
		}
		data.c[i] = static_cast<char>(c);
	}

	return SInt((negative ? -1 : 1) * le64toh(data.i));
}

SVal TlvSerializer::decodeFloat(std::istream& is)
{
	auto token = static_cast<Token>(is.get());
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
				auto c = is.get();
				if (c == -1)
				{
					throw std::runtime_error("Unexpect out of data during parse number value");
				}
				i = static_cast<char>(c);
			}

			return SFloat(le32toh(data.f));
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
				auto c = is.get();
				if (c == -1)
				{
					throw std::runtime_error("Unexpect out of data during parse number value");
				}
				data.c[i] = static_cast<char>(c);
			}

			return SFloat(le64toh(data.f));
		}
		default: throw std::runtime_error("Wrong token for float value");
	}
}

void TlvSerializer::encodeNull(std::ostream& os, const SVal& value)
{
	os.put(static_cast<char>(Token::NULL));
}

void TlvSerializer::encodeBool(std::ostream& os, const SVal& value)
{
	os.put(static_cast<char>(value.as<SBool>() ? Token::TRUE : Token::FALSE));
}

void TlvSerializer::encodeString(std::ostream& os, const SVal& value)
{
	os.put(static_cast<char>(Token::STRING));
	os << value.as<SStr>().value();
	os.put(static_cast<char>(Token::END));
}

void TlvSerializer::encodeKey(std::ostream& os, const std::string& key)
{
	os.put(static_cast<char>(Token::STRING));
	os << key;
	os.put(static_cast<char>(Token::END));
}

void TlvSerializer::encodeBinary(std::ostream& os, const SVal& value)
{
	const auto& bin = value.as<SBinary>();

	if (bin.size() <= std::numeric_limits<uint8_t>::max())
	{
		os.put(static_cast<char>(Token::BINARY_255));
		os.put(static_cast<uint8_t>(bin.size()));
	}
	else if (bin.size() <= std::numeric_limits<uint16_t>::max())
	{
		os.put(static_cast<char>(Token::BINARY_64K));
		auto size_le = htole16(static_cast<uint16_t>(bin.size()));
		for (size_t i = 0; i < sizeof(size_le); i++)
		{
			os.put(reinterpret_cast<uint8_t*>(&size_le)[i]);
		}
	}
	else if (bin.size() <= std::numeric_limits<uint32_t>::max())
	{
		os.put(static_cast<char>(Token::BINARY_4G));
		auto size_le = htole32(static_cast<uint32_t>(bin.size()));
		for (size_t i = 0; i < sizeof(size_le); i++)
		{
			os.put(reinterpret_cast<uint8_t*>(&size_le)[i]);
		}
	}
	else
	{
		throw std::runtime_error("Too long binary data");
	}

	os.write(bin.data(), bin.size());
}

void TlvSerializer::encodeNumber(std::ostream& os, const SVal& value)
{
	if (value.is<SInt>())
	{
		if (value.as<SInt>().value() == 0)
		{
			os.put(static_cast<char>(Token::ZERO));
			return;
		}

		int bytes = 0;

		bool negative = value.as<SInt>().value() < 0;

		auto absValue = static_cast<uint64_t>(llabs(value.as<SInt>().value()));

		if (absValue == 0)
		{
			os.put(static_cast<char>(Token::ZERO));
			bytes = 0;
		}
		else if (absValue <= UINT8_MAX)
		{
			os.put(static_cast<char>(negative ? Token::NEG_INT_8 : Token::POS_INT_8));
			bytes = 1;
		}
		else if (absValue <= UINT16_MAX)
		{
			os.put(static_cast<char>(negative ? Token::NEG_INT_16 : Token::POS_INT_16));
			bytes = 2;
		}
		else if (absValue <= UINT32_MAX)
		{
			os.put(static_cast<char>(negative ? Token::NEG_INT_32 : Token::POS_INT_32));
			bytes = 4;
		}
		else if (absValue <= UINT64_MAX)
		{
			os.put(static_cast<char>(negative ? Token::NEG_INT_64 : Token::POS_INT_64));
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
			os.put(data.c[i]);
		}
		return;
	}
	if (value.is<SFloat>())
	{
		auto i64 = static_cast<int64_t>(value.as<SFloat>().value());
		if (static_cast<decltype(value.as<SFloat>().value())>(i64) == value.as<SFloat>().value())
		{
			encodeNumber(os, i64);
			return;
		}

		auto f32 = static_cast<float_t>(value.as<SFloat>().value());
		if (static_cast<decltype(value.as<SFloat>().value())>(f32) == value.as<SFloat>().value())
		{
			char data[4];

			f32 = htole32(f32);
			memcpy(data, &f32, sizeof(data));

			os.put(static_cast<char>(Token::FLOAT_32));
			for (char i : data)
			{
				os.put(i);
			}
			return;
		}

		auto f64 = static_cast<double_t>(value.as<SFloat>().value());

		char data[8];

		f64 = htole64(f64);
		memcpy(data, &f64, sizeof(data));

		os.put(static_cast<char>(Token::FLOAT_64));
		for (char i : data)
		{
			os.put(i);
		}
	}
}

void TlvSerializer::encodeArray(std::ostream& os, const SVal& value)
{
	const auto& array = value.as<SArr>();

	os.put(static_cast<char>(Token::ARRAY));
	std::for_each(array.cbegin(), array.cend(),
		[this, &os]
		(const SVal& val)
		{
			encodeValue(os, val);
		}
	);
	os.put(static_cast<char>(Token::ARRAY_END));
}

void TlvSerializer::encodeObject(std::ostream& os, const SVal& value)
{
	const auto& object = value.as<SObj>();

	os.put(static_cast<char>(Token::OBJECT));
	std::for_each(object.cbegin(), object.cend(),
		[this, &os]
		(const std::pair<std::string, SVal>& element)
		{
			encodeKey(os, element.first);
			encodeValue(os, element.second);
		}
	);
	os.put(static_cast<char>(Token::OBJECT_END));
}

void TlvSerializer::encodeValue(std::ostream& os, const SVal& value)
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
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}

void TlvSerializer::putUtf8Symbol(SStr& str, uint32_t symbol)
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
