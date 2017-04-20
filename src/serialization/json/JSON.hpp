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

// JSON.hpp


#pragma once

#include <string>
#include "../SBool.hpp"
#include "../SObj.hpp"
#include "../SStr.hpp"
#include "../SVal.hpp"
#include "../SArr.hpp"
#include "../SNull.hpp"
#include "../SNum.hpp"

class JSON
{
private:
	static void skipSpaces(std::istringstream &iss);

	static SNull* decodeNull(std::istringstream &iss);
	static SBool* decodeBool(std::istringstream &iss);

	static uint32_t decodeEscaped(std::istringstream &iss);
	static void putUtf8Symbol(SStr &str, uint32_t symbol);
	static SStr* decodeString(std::istringstream &iss);

	static SNum* decodeNumber(std::istringstream &iss);

	static SArr* decodeArray(std::istringstream &iss);
	static SObj* decodeObject(std::istringstream &iss);

	static SVal* decodeValue(std::istringstream &iss);


	static void encodeNull(const SNull* value, std::ostringstream &oss);
	static void encodeBool(const SBool* value, std::ostringstream &oss);

	static void encodeString(const SStr* value, std::ostringstream &oss);

	static void encodeNumber(const SNum* value, std::ostringstream &oss);

	static void encodeArray(const SArr* value, std::ostringstream &oss);
	static void encodeObject(const SObj* value, std::ostringstream &oss);

	static void encodeValue(const SVal* value, std::ostringstream &oss);

public:
	static SVal* decode(std::string &data);

	static std::string encode(const SVal* value);
};
