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

// JsonSerializer.hpp


#pragma once

#include "Serializer.hpp"

#include "SNull.hpp"
#include "SBool.hpp"
#include "SNum.hpp"
#include "SStr.hpp"
#include "SBinary.hpp"
#include "SObj.hpp"
#include "SArr.hpp"
#include "SVal.hpp"

#include <sstream>
#include <string>

class JsonSerializer : public Serializer
{
public:
	static const uint32_t ESCAPED_UNICODE = 1u<<31;

	DECLARE_SERIALIZER(JsonSerializer);

	std::istringstream _iss;
	std::ostringstream _oss;

	void skipSpaces();

	SNull* decodeNull();
	SBool* decodeBool();

	uint32_t decodeEscaped();
	void putUtf8Symbol(SStr &str, uint32_t symbol);
	SStr* decodeString();

	SBinary* decodeBinary();

	SNum* decodeNumber();

	SArr* decodeArray();
	SObj* decodeObject();

	SVal* decodeValue();


	void encodeNull(const SNull* value);
	void encodeBool(const SBool* value);

	void encodeString(const std::string& string);
	void encodeString(const SStr* value);
	void encodeBinary(const SBinary* value);

	void encodeNumber(const SNum* value);

	void encodeArray(const SArr* value);
	void encodeValue(const SVal* value);

	void encodeObject(const SObj* value);
};
