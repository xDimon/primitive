// Copyright © 2019 Dmitriy Khaustov
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
// File created on: 2019.04.24

// Amf3Serializer.cpp


#include "Amf3Serializer.hpp"

#include <cstring>
#include <zconf.h>
#include <zlib.h>
//#include "SBool.hpp"
#include "../SObj.hpp"
#include "../SStr.hpp"
//#include "SVal.hpp"
#include "../SArr.hpp"
//#include "SNull.hpp"
//#include "SNum.hpp"
#include "../SInt.hpp"
#include "../SFloat.hpp"
#include "../SBinary.hpp"
#include "../../compression/CompressorFactory.hpp"
#include "Amf3Exeption.hpp"

REGISTER_SERIALIZER(amf3, Amf3Serializer);

#ifdef NULL
#undef NULL
#endif

enum class Token : uint8_t {
	UNDEFINED		= 0,
	NULL			= 1,
	FALSE			= 2,
	TRUE			= 3,
	INTEGER			= 4,
	FLOAT			= 5,
	STRING			= 6,
	__XMLDOC		= 7,
	__DATE			= 8,
	ARRAY			= 9,
	OBJECT			= 10,
	__XML			= 11,
	BINARY			= 12,
	__VECTOR_INT	= 13,
	__VECTOR_UINT	= 14,
	__VECTOR_DOUBLE	= 15,
	__VECTOR_OBJECT	= 16,
	DICTIONARY		= 17
};

SVal Amf3Serializer::decode(std::istream& is)
{
	if (is.eof())
	{
		throw Amf3Exeption("No data for parsing");
	}

	Amf3Context ctx(is);

	SVal value;

	try
	{
		if ((_flags & HEADLESS) == 0)
		{
			decodeVersion(ctx);
			decodeHeaders(ctx);
			decodeMessages(ctx);
		}

		value = decodeValue(ctx);

		// Проверяем лишние данные в конце
		if (!ctx.is.eof() && (_flags & Serializer::STRICT) != 0)
		{
			throw Amf3Exeption("Redundant bytes after parsed data");
		}
	}
	catch (Amf3Exeption& exception)
	{
		throw Amf3Exeption(std::string("Can't decode AMF ← ") + exception.what());
	}

	return value;
}

void Amf3Serializer::encode(std::ostream& os, const SVal& value)
{
	Amf3Context ctx(os);

	try
	{
		if ((_flags & HEADLESS) == 0)
		{
//			encodeVersion(ctx);
//			encodeHeaders(ctx);
//			encodeMessages(ctx);
		}

		encodeValue(ctx, value);
	}
	catch (Amf3Exeption& exception)
	{
		throw Amf3Exeption(std::string("Can't encode into AMF ← ") + exception.what());
	}
}

void Amf3Serializer::decodeVersion(Amf3Context& ctx)
{
	if (ctx.is.eof())
	{
		throw Amf3Exeption("Unexpect out of data during parse version");
	}

	uint16_t version;

	ctx.is.read(reinterpret_cast<char*>(&version), sizeof(version));

	if (version == 0x00)
	{
		throw Amf3Exeption("Unsupported version '" + std::to_string(version) + "'");
	}

	if (version != 0x03)
	{
		throw Amf3Exeption("Invalid version '" + std::to_string(version) + "'");
	}
}

void Amf3Serializer::decodeHeaders(Amf3Context& ctx)
{
	if (ctx.is.eof())
	{
		throw Amf3Exeption("Unexpect out of data during parse headers");
	}

	uint16_t headerCount;

	ctx.is.read(reinterpret_cast<char*>(&headerCount), sizeof(headerCount));

	while (headerCount--)
	{
		auto headerName = decodeHeadlessString(ctx);

//		bool mustUnderstand =
			ctx.is.get();

		uint32_t headerLength;

		ctx.is.read(reinterpret_cast<char*>(&headerLength), sizeof(headerLength));

		auto headerValue = decodeValue(ctx);

//		var header = new a3d.AMFHeader(headerName, mustUnderstand, headerValue);
//		response.headers.push(header);
	}
}

void Amf3Serializer::decodeMessages(Amf3Context& ctx)
{
	if (ctx.is.eof())
	{
		throw Amf3Exeption("Unexpect out of data during parse messages");
	}

	uint16_t messageCount;

	ctx.is.read(reinterpret_cast<char*>(&messageCount), sizeof(messageCount));

	while (messageCount--)
	{
		auto targetURI = decodeHeadlessString(ctx);
		auto responseURI = decodeHeadlessString(ctx);

		uint32_t messageLength;

		ctx.is.read(reinterpret_cast<char*>(&messageLength), sizeof(messageLength));

		auto messageBody = decodeValue(ctx);

//		var messages = new a3d.AMFMessage(targetURI, responseURI, messageBody);
//		response.messages.push(messages);
	}
}

SVal Amf3Serializer::decodeValue(Amf3Context& ctx)
{
	if (ctx.is.eof())
	{
		throw Amf3Exeption("Unexpect out of data during parse value");
	}

	auto token = static_cast<Token>(ctx.is.peek());
	switch (token)
	{
		case Token::UNDEFINED:
			return decodeUndefined(ctx);

		case Token::NULL:
			return decodeNull(ctx);

		case Token::FALSE:
		case Token::TRUE:
			return decodeBool(ctx);

		case Token::INTEGER:
			return decodeInteger(ctx);

		case Token::FLOAT:
			return decodeFloat(ctx);

		case Token::STRING:
			return decodeString(ctx);

		case Token::ARRAY:
			return decodeArray(ctx);

		case Token::DICTIONARY:
			return decodeDictionary(ctx);

		case Token::BINARY:
			return decodeBinary(ctx);

		case Token::OBJECT:
			return decodeObject(ctx);

		case Token::__XMLDOC:
		case Token::__DATE:
		case Token::__XML:
		case Token::__VECTOR_INT:
		case Token::__VECTOR_UINT:
		case Token::__VECTOR_DOUBLE:
		case Token::__VECTOR_OBJECT:
			throw Amf3Exeption("Unsupported token at parse value");

		default:
			throw Amf3Exeption("Unknown token at parse value");
	}
}

SVal Amf3Serializer::decodeArray(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::ARRAY)
	{
		throw Amf3Exeption("Wrong token for open array");
	}

	enum class Type
	{
		REFFERENCE	= 0,
		VALUE		= 1
	} type;
	size_t length;

	try
	{
		uint32_t tmp = decodeHeadlessInteger(ctx);
		type = static_cast<Type>(tmp & 0x01);
		length = tmp >> 1;
	}
	catch (const std::exception& exception)
	{
		throw Amf3Exeption("Can't parse size of array");
	}

	SArr arr;

	if (type == Type::VALUE)
	{
		// Associative portion
		for (;;)
		{
			try
			{
				auto name = decodeHeadlessString(ctx);
				if (name.empty())
				{
					break;
				}
				auto value = decodeValue(ctx);
			}
			catch (const std::exception& exception)
			{
				throw Amf3Exeption(std::string("Can't parse key-value in associative portion of array ← ") + exception.what());
			}

			// TODO do something with associative pair
		}

		// Dense portion
		while (length--)
		{
			try
			{
				arr.emplace_back(decodeValue(ctx));
			}
			catch (const std::exception& exception)
			{
				throw Amf3Exeption("Can't parse element #" + std::to_string(arr.size()) + " in dense portion of array ← " + exception.what());
			}
		}
	}
	else
	{
		throw Amf3Exeption("Array reference not supported yet");
	}

	return arr;
}

SVal Amf3Serializer::decodeObject(Amf3Context& ctx)
{
	auto c = static_cast<Token>(ctx.is.get());
	if (c != Token::OBJECT)
	{
		throw Amf3Exeption("Wrong token for open object");
	}

	enum class Type
	{
		REFFERENCE	= 0,
		TRAITS_REF  = 1,
		TRAITS_EXT  = 2,
		TRAITS		= 3
	} type;
	size_t index = 0;
	Amf3Traits traits;

	try
	{
		uint32_t tmp = decodeHeadlessInteger(ctx);

		// *******0
		if ((tmp & 0b0000'0001) == 0)
		{
			type = Type::REFFERENCE;
			index = tmp >> 1;
		}
		// ******01
		else if ((tmp & 0b0000'0011) == 0b01)
		{
			type = Type::TRAITS_REF;
			index = tmp >> 2;
			traits = ctx.getTraits(index);
		}
		// *****111
		else if ((tmp & 0b0000'0111) == 0b111)
		{
			type = Type::TRAITS_EXT;
			traits.className = decodeHeadlessString(ctx);
			traits.externalizable = true;
			ctx.putTraits(traits);
		}
		// *****011
		else
		{
			type = Type::TRAITS;
			traits.className = decodeHeadlessString(ctx);
			// ****1***
			if ((tmp & 0b0000'1000) != 0)
			{
				traits.dynamic = true;
				uint32_t sealedMembersCount = tmp >> 4;
				for (uint32_t i = 0; i < sealedMembersCount; ++i)
				{
					auto attr = decodeHeadlessString(ctx);
					traits.attributes.push_back(attr);
				}
			}
			ctx.putTraits(traits);
		}
	}
	catch (const std::exception& exception)
	{
		throw Amf3Exeption(std::string("Can't parse header of object ← ") + exception.what());
	}

	SObj obj;

	if (traits.externalizable)
	{
//		obj = read_for_(traits.className, ctx);
		return obj;
	}

	for (const auto& name : traits.attributes)
	{
		obj.emplace(name, decodeValue(ctx));
	}

	if (!traits.dynamic)
	{
		return obj;
	}

	std::string prevKey;
	for (;;)
	{
		std::string key;
		try
		{
			key = decodeHeadlessString(ctx);
		}
		catch (const Amf3Exeption& exception)
		{
			throw Amf3Exeption(std::string() + "Can't parse key of object field (prev field: " + prevKey + ") ← " + exception.what());
		}

		if (key.empty())
		{
			return obj;
		}

		SVal value;
		try
		{
			value = decodeValue(ctx);
		}
		catch (const std::exception& exception)
		{
			throw Amf3Exeption("Can't parse value of object field '" + key + "' ← " +  + exception.what());
		}

		prevKey = key;
		obj.emplace(std::move(key), std::move(value));
	}
}

SVal Amf3Serializer::decodeDictionary(Amf3Context& ctx)
{
	auto c = static_cast<Token>(ctx.is.get());
	if (c != Token::DICTIONARY)
	{
		throw Amf3Exeption("Wrong token for open dictionary");
	}

	enum class Type
	{
		REFFERENCE	= 0,
		VALUE		= 1
	} type;
	size_t length;

	try
	{
		uint32_t tmp = decodeHeadlessInteger(ctx);
		type = static_cast<Type>(tmp & 0x01);
		length = tmp >> 1;
	}
	catch (const std::exception& exception)
	{
		throw Amf3Exeption("Can't parse length of dictionary");
	}

	bool weakness = ctx.is.get() != 0x00;
	(void)weakness;

	SObj obj;

	if (type == Type::VALUE)
	{
		while (length--)
		{
			std::string key;
			try
			{
				key = decodeHeadlessString(ctx);
			}
			catch (const Amf3Exeption& exception)
			{
				throw Amf3Exeption(std::string("Can't parse key of dictionary record ← ") + exception.what());
			}

			SVal value;
			try
			{
				decodeValue(ctx);
			}
			catch (const std::exception& exception)
			{
				throw Amf3Exeption(std::string("Can't parse value of dictionary record ← ") + exception.what());
			}

			obj.emplace(std::move(key), std::move(value));
		}

		return obj;
	}
	else
	{
		throw Amf3Exeption("Dictionary reference not supported yet");
	}
}

std::string Amf3Serializer::decodeHeadlessString(Amf3Context& ctx)
{
	static int cnt = 0;
	++cnt;

	enum class Type
	{
		REFFERENCE	= 0,
		VALUE		= 1
	} type;
	size_t index = -1;
	size_t length = 0;
	try
	{
		uint32_t tmp = decodeHeadlessInteger(ctx);

		if (tmp & 0x01)
		{
			type = Type::VALUE;
			length = tmp >> 1;
		}
		else
		{
			type = Type::REFFERENCE;
			index = tmp >> 1;
		}
	}
	catch (const std::exception& exception)
	{
		throw Amf3Exeption("Can't parse header of string");
	}

	std::string str;

	if (type == Type::VALUE)
	{
		while (length)
		{
			length -= putUtf8Symbol(str, decodeUtf8Symbol(ctx));
		}

		if (!str.empty())
		{
			if (ctx.getIndex(str) == -1)
			{
				ctx.putSring(str);
			}
		}
	}
	else
	{
		str = ctx.getString(index);

		if (!str.empty())
		{
			if (ctx.getIndex(str) == -1)
			{
				ctx.putSring(str);
			}
		}
	}

	return str;
}

uint32_t Amf3Serializer::decodeHeadlessInteger(Amf3Context& ctx)
{
	uint32_t value = 0;
	int byte = 4;

	for (;;)
	{
		if (ctx.is.eof())
		{
			throw Amf3Exeption("Unexpected out of data during parse u29-integer value");
		}

		auto u8 = static_cast<uint8_t>(ctx.is.get());
		if (--byte)
		{
			value = (value << 7) | (u8 & 0x7F);
			if ((u8 & 0x80) == 0)
			{
				break;
			}
		}
		else
		{
			value = (value << 8) | (u8 & 0xFF);
			break;
		}
	}

	return value;
}

uint32_t Amf3Serializer::decodeUtf8Symbol(Amf3Context& ctx)
{
	if (ctx.is.eof())
	{
		throw Amf3Exeption("Unexpected out of data during parse utf8 symbol");
	}

	auto c = ctx.is.get();

	// Однобайтовый символ
	if (c < 0b1000'0000)
	{
		return c;
	}

	// Некорректный начальный байт
	if (c > 0b1111'1101)
	{
		throw Amf3Exeption("Bad symbol in string value: prohibited utf8 byte");
	}

	if (c <= 0b11110111)
	{
		int bytes;
		uint32_t utf8Symbol = 0;

		if ((c & 0b11111000) == 0b11110000)
		{
			bytes = 4;
			utf8Symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b111);
		}
		else if ((c & 0b11110000) == 0b11100000)
		{
			bytes = 3;
			utf8Symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b1111);
		}
		else if ((c & 0b11100000) == 0b11000000)
		{
			bytes = 2;
			utf8Symbol = static_cast<uint8_t>(c) & static_cast<uint8_t>(0b11111);
		}
		else
		{
			if (_flags & STRICT)
			{
				throw Amf3Exeption("Bad symbol in string value: invalid utf8 byte");
			}
			else
			{
				return 0x001A;
			}
		}
		while (--bytes > 0)
		{
			if (ctx.is.eof())
			{
				throw Amf3Exeption("Unxpected end of data during parse utf8 symbol", ctx.is.tellg());
			}

			c = ctx.is.peek();
			if ((c & 0b11000000) != 0b10000000)
			{
				throw Amf3Exeption("Bad symbol in string value: invalid utf8 byte", ctx.is.tellg());
			}
			ctx.is.ignore();

			utf8Symbol = (utf8Symbol << 6) | (c & 0b0011'1111);
		}
		return utf8Symbol;
	}

	throw Amf3Exeption("Bad symbol in string value: prohibited utf8 byte");
}

SVal Amf3Serializer::decodeString(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::STRING)
	{
		throw Amf3Exeption("Wrong token for open string");
	}

	return SStr(decodeHeadlessString(ctx));
}

SVal Amf3Serializer::decodeBinary(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::BINARY)
	{
		throw Amf3Exeption("Wrong token for open binary");
	}

	enum class Type
	{
		REFFERENCE	= 0,
		VALUE		= 1
	} type;
	size_t length;

	try
	{
		uint32_t tmp = decodeHeadlessInteger(ctx);
		type = static_cast<Type>(tmp & 0x01);
		length = tmp >> 1;
	}
	catch (const std::exception& exception)
	{
		throw Amf3Exeption("Can't parse length of binary");
	}

	SBinary bin;
	bin.reserve(length);
	auto it = bin.begin();

	if (type == Type::VALUE)
	{
		while (length--)
		{
			if (ctx.is.eof())
			{
				throw Amf3Exeption("Unexpected out of data during parse binary value");
			}

			bin.insert(it++, ctx.is.get());
		}
	}
	else
	{
		throw Amf3Exeption("Binary referencece not supported yet");
	}

	return bin;
}

SVal Amf3Serializer::decodeUndefined(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::UNDEFINED)
	{
		throw Amf3Exeption("Wrong token for undefined value");
	}

	return SVal();
}

SVal Amf3Serializer::decodeNull(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::NULL)
	{
		throw Amf3Exeption("Wrong token for null value");
	}

	return SNull();
}

SVal Amf3Serializer::decodeBool(Amf3Context& ctx)
{
	auto c = static_cast<Token>(ctx.is.get());
	switch (c)
	{
		case Token::TRUE:
			return SBool(true);

		case Token::FALSE:
			return SBool(false);

		default: throw Amf3Exeption("Wrong token for boolean value");
	}
}

SVal Amf3Serializer::decodeInteger(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::INTEGER)
	{
		throw Amf3Exeption("Wrong token for integer value");
	}

	return SInt(decodeHeadlessInteger(ctx));
}

SVal Amf3Serializer::decodeFloat(Amf3Context& ctx)
{
	if (static_cast<Token>(ctx.is.get()) != Token::FLOAT)
	{
		throw Amf3Exeption("Wrong token for float value");
	}

	union
	{
		double flt;
		char byte[8];
	} data;

	for (auto i = 0u; i < sizeof(double); i++)
	{
		if (ctx.is.eof())
		{
			throw Amf3Exeption("Unexpect out of data during parse float value");
		}

		data.byte[i] = ctx.is.get();
	}

	return SFloat(be64toh(data.flt));
}

void Amf3Serializer::encodeHeadlessString(Amf3Context& ctx, const std::string& str)
{
	int32_t index = ctx.getIndex(str);
	if (index == -1)
	{
		static int cnt = 0;
		++cnt;

		uint32_t lengthAndType = (str.size() << 1) | 0x01;
		encodeHeadlessInteger(ctx, lengthAndType);
		if (!str.empty())
		{
			ctx.os.write(str.data(), str.size());
			ctx.putSring(str);
		}
	}
	else
	{
		uint32_t indexAndType = (index << 1);
		encodeHeadlessInteger(ctx, indexAndType);
	}
}

void Amf3Serializer::encodeUndefined(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::UNDEFINED));
}

void Amf3Serializer::encodeNull(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::NULL));
}

void Amf3Serializer::encodeBool(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(value.as<SBool>() ? Token::TRUE : Token::FALSE));
}

void Amf3Serializer::encodeString(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::STRING));

	encodeHeadlessString(ctx, value.as<SStr>().value());
}

void Amf3Serializer::encodeBinary(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::BINARY));

	const auto& bin = value.as<SBinary>();

	// Size and reference flag
	uint32_t lengthAndType = (bin.size() << 1) | 0x01;
	encodeHeadlessInteger(ctx, lengthAndType);

	ctx.os.write(bin.data(), bin.size());
}

void Amf3Serializer::encodeHeadlessInteger(Amf3Context& ctx, int64_t intVal)
{
	static int cnt = 0;
	++cnt;

	intVal = static_cast<uint32_t>(intVal);

	if (intVal <= 0x0000007F) // 0xxxxxxx
	{
		ctx.os.put(intVal & 0x7F);
	}
	else if (intVal <= 0x00003FFF) // 1xxxxxxx 0xxxxxxx
	{
		ctx.os.put(((intVal>>7) & 0x7F) | 0x80);
		ctx.os.put(intVal & 0x7F);
	}
	else if (intVal <= 0x001FFFFF) // 1xxxxxxx 1xxxxxxx 0xxxxxxx
	{
		ctx.os.put(((intVal>>14) & 0x7F) | 0x80);
		ctx.os.put(((intVal>>7) & 0x7F) | 0x80);
		ctx.os.put(intVal & 0x7F);
	}
	else if (intVal <= 0x3FFFFFFF) // 1xxxxxxx 1xxxxxxx 1xxxxxxx xxxxxxxx
	{
		ctx.os.put(((intVal>>22) & 0x7F) | 0x80);
		ctx.os.put(((intVal>>15) & 0x7F) | 0x80);
		ctx.os.put(((intVal>>8) & 0x7F) | 0x80);
		ctx.os.put(intVal & 0xFF);
	}
	else // range exception
	{
		throw Amf3Exeption("Too large integer value");
	}
}

void Amf3Serializer::encodeHeadlessFloat(Amf3Context& ctx, double fltValue)
{
	union
	{
		double flt;
		char byte[8];
	} data;

	data.flt = htobe64(fltValue);

	for (auto i = 0u; i < sizeof(double); i++)
	{
		ctx.os.put(data.byte[i]);
	}
}

void Amf3Serializer::encodeNumber(Amf3Context& ctx, const SVal& value)
{
	if (value.is<SInt>())
	{
		auto intVal = value.as<SInt>().value();
		if (intVal >= 1<<29)
		{
			encodeNumber(ctx, SFloat(intVal));
			return;
		}
	}

	if (value.is<SInt>())
	{
		ctx.os.put(static_cast<char>(Token::INTEGER));

		auto intVal = static_cast<uint32_t>(value.as<SInt>().value());
		int byte = 4;

		for (;;)
		{
			auto u8 = static_cast<uint8_t>(intVal & 0x7F);

			intVal >>= 7;

			if (--byte && intVal)
			{
				ctx.os.put(static_cast<char>(0x80 | u8));
			}
			else
			{
				ctx.os.put(static_cast<char>(u8));
				return;
			}
		}
	}

	if (value.is<SFloat>())
	{
		auto i64 = static_cast<int64_t>(value.as<SFloat>().value());
		if (i64 < 1<<29 && static_cast<SFloat::type>(i64) == value.as<SFloat>().value())
		{
			encodeNumber(ctx, SInt(i64));
			return;
		}

		ctx.os.put(static_cast<char>(Token::FLOAT));

		union
		{
			double flt;
			char byte[8];
		} data;

		data.flt = htobe64(value.as<SFloat>().value());

		for (auto i = 0u; i < sizeof(double); i++)
		{
			ctx.os.put(data.byte[i]);
		}
	}
}

void Amf3Serializer::encodeArray(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::ARRAY));

	const auto& array = value.as<SArr>();

	// Size and reference flag
	uint32_t lengthAndType = (array.size() << 1) | 0x01;
	encodeHeadlessInteger(ctx, lengthAndType);

	// Associative portion
	encodeHeadlessString(ctx, ""); // utf8-empty

	// Dense portion
	std::for_each(array.cbegin(), array.cend(),
		[this, &ctx]
		(const SVal& val)
		{
			encodeValue(ctx, val);
 		}
	);
}

void Amf3Serializer::encodeObject(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::OBJECT));

	const auto& object = value.as<SObj>();

	uint32_t sealedMembersCount = 0;

	// U29-traits = 0b0011 = 0x03
	uint32_t traitMarker = sealedMembersCount << 4 | 0x03;
	// dynamic marker = 0b1000 = 0x08
	traitMarker |= 0x08;

	encodeHeadlessInteger(ctx, traitMarker);

	// class-name
	encodeHeadlessString(ctx, "");

	// dynamic-members = UTF-8-vr value-type
	for (const auto& it : object)
	{
//		if (it.first == "avatar_alex")
//		{
//			[]{}();
//		}
//
		// key
		encodeHeadlessString(ctx, it.first);

		// value
		encodeValue(ctx, it.second);
	}

	// final dynamic member = UTF-8-empty
	encodeHeadlessString(ctx, "");
}

void Amf3Serializer::encodeDictionary(Amf3Context& ctx, const SVal& value)
{
	ctx.os.put(static_cast<char>(Token::DICTIONARY));

	const auto& object = value.as<SObj>();

	// Size and reference flag
	uint32_t lengthAndType = (object.size() << 1) | 0x01;
	encodeHeadlessInteger(ctx, lengthAndType);

	// Weakness
	ctx.os.put(0x00); // strong

	std::for_each(object.cbegin(), object.cend(),
		[this, &ctx]
		(const std::pair<std::string, SVal>& element)
		{
			encodeHeadlessString(ctx, element.first);
			encodeValue(ctx, element.second);
		}
	);
}

void Amf3Serializer::encodeValue(Amf3Context& ctx, const SVal& value)
{
	switch (value.type())
	{
		case SVal::Type::Null:
			encodeNull(ctx, value);
			break;
		case SVal::Type::Bool:
			encodeBool(ctx, value);
			break;
		case SVal::Type::Integer:
			encodeNumber(ctx, value);
			break;
		case SVal::Type::Float:
			encodeNumber(ctx, value);
			break;
		case SVal::Type::String:
			encodeString(ctx, value);
			break;
		case SVal::Type::Binary:
			encodeBinary(ctx, value);
			break;
		case SVal::Type::Array:
			encodeArray(ctx, value);
			break;
		case SVal::Type::Object:
			encodeObject(ctx, value);
			break;
		case SVal::Type::Undefined:
			throw Amf3Exeption("Undefined value");
		default:
			throw Amf3Exeption("Unknown value type");
	}
}

size_t Amf3Serializer::putUtf8Symbol(std::string& str, uint32_t symbol)
{
	if (symbol <= 0b0111'1111) // 7bit -> 1byte
	{
		str.push_back(symbol);
		return 1;
	}
	else if (symbol <= 0b0111'1111'1111) // 11bit -> 2byte
	{
		str.push_back(0b1100'0000 | (0b0001'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
		return 2;
	}
	else if (symbol <= 0b1111'1111'1111'1111) // 16bit -> 3byte
	{
		str.push_back(0b1110'0000 | (0b0000'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
		return 3;
	}
	else if (symbol <= 0b0001'1111'1111'1111'1111'1111) // 21bit -> 4byte
	{
		str.push_back(0b1111'0000 | (0b0000'0111 & (symbol >> 18)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
		return 4;
	}
//	else if (symbol <= 0b0011'1111'1111'1111'1111'1111'1111) // 26bit -> 5byte
//	{
//		str.push_back(0b1111'1000 | (0b0000'0011 & (symbol >> 24)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
//	}
//	else if (symbol <= 0b0111'1111'1111'1111'1111'1111'1111'1111) // 31bit -> 6byte
//	{
//		str.push_back(0b1111'1100 | (0b0000'0001 & (symbol >> 30)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 24)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 18)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 12)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 6)));
//		str.push_back(0b1000'0000 | (0b0011'1111 & (symbol >> 0)));
//	}

	throw Amf3Exeption("Bad unicode symbol");
}
