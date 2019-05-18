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

// JsonSerializer.hpp


#pragma once

#include "Serializer.hpp"

#include <utility>
#include <iostream>

#include "SNull.hpp"
#include "SBool.hpp"
#include "SNum.hpp"
#include "SStr.hpp"
#include "SBinary.hpp"
#include "SObj.hpp"
#include "SArr.hpp"
#include "SBase.hpp"

class JsonSerializer final: public Serializer
{
public:
	static const uint32_t ESCAPED_UNICODE = 1u<<31;
	static const uint32_t ESCAPED_SLASH = 1u<<30;

private:
	void skipSpaces(std::istream& is);

	SVal decodeNull(std::istream& is);
	SVal decodeBool(std::istream& is);

	uint32_t decodeEscaped(std::istream& is);
	void putUnicodeSymbolAsUtf8(SStr& str, uint32_t symbol);
	SVal decodeString(std::istream& is);

	SVal decodeBinary(std::istream& is);

	SVal decodeNumber(std::istream& is);

	SVal decodeArray(std::istream& is);
	SVal decodeObject(std::istream& is);

	SVal decodeValue(std::istream& is);

	void encodeValue(std::ostream &os, const SVal& value);

	void encodeNull(std::ostream &os, const SVal& value);
	void encodeBool(std::ostream &os, const SVal& value);

	void encodeString(std::ostream &os, const SVal& value);
	void encodeBinary(std::ostream &os, const SVal& value);

	void encodeNumber(std::ostream &os, const SVal& value);

	void encodeArray(std::ostream &os, const SVal& value);
	void encodeObject(std::ostream &os, const SVal& value);

DECLARE_SERIALIZER(JsonSerializer);
};

class JsonParseExeption final: public std::exception
{
	std::string _msg;

public:
	JsonParseExeption() = delete; // Default-constructor
	JsonParseExeption(JsonParseExeption&&) noexcept = default; // Move-constructor
	JsonParseExeption(const JsonParseExeption&) = delete; // Copy-constructor
	~JsonParseExeption() override = default; // Destructor
	JsonParseExeption& operator=(JsonParseExeption&&) noexcept = delete; // Move-assignment
	JsonParseExeption& operator=(JsonParseExeption const&) = delete; // Copy-assignment

	JsonParseExeption(std::string msg, size_t pos, std::istream& is)
	: _msg(std::move(msg))
	{
		is.clear(std::istream::goodbit);
		is.seekg(pos);

		std::string nearPos;
		while (!is.eof() && is.peek() != -1 && nearPos.size() < 20)
		{
			auto c = is.get();
			switch (c)
			{
				case '\t': nearPos += "\\t"; break;
				case '\n': nearPos += "\\n"; break;
				case '\r': nearPos += "\\r"; break;
				case '\b': nearPos += "\\b"; break;
				case '\f': nearPos += "\\f"; break;
				default: nearPos.push_back(c);
			}
		}

		_msg += " at position " + std::to_string(pos) + " (remain: '" + nearPos + "')";
	}

	JsonParseExeption(std::string msg, size_t pos)
	: _msg(std::move(msg))
	{
		_msg += " at position " + std::to_string(pos);
	}

	JsonParseExeption(std::string msg)
	: _msg(std::move(msg))
	{
	}

	const char* what() const noexcept override
	{
		return _msg.c_str();
	}
};
