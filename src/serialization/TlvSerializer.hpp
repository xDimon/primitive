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

// TlvSerializer.hpp


#pragma once


#include "Serializer.hpp"

#include "SBool.hpp"
#include "SObj.hpp"
#include "SStr.hpp"
#include "SVal.hpp"
#include "SArr.hpp"
#include "SNull.hpp"
#include "SNum.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SBinary.hpp"

#include <sstream>
#include <string>

class TlvSerializer final: public Serializer
{
private:
	std::istringstream _iss;
	std::ostringstream _oss;

	SVal decodeNull();
	SVal decodeBool();

	void putUtf8Symbol(SStr &str, uint32_t symbol);
	SVal decodeString();
	SVal decodeBinary();

	SVal decodeInteger();
	SVal decodeFloat();

	SVal decodeArray();
	SVal decodeObject();

	SVal decodeValue();

	void encodeNull(const SVal& value);
	void encodeBool(const SVal& value);

	void encodeString(const SVal& value);
	void encodeBinary(const SVal& value);

	void encodeNumber(const SVal& value);

	void encodeArray(const SVal& value);
	void encodeKey(const std::string& key);
	void encodeObject(const SVal& value);

	void encodeValue(const SVal& value);

DECLARE_SERIALIZER(TlvSerializer);
};
