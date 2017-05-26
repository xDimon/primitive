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
// File created on: 2017.05.26

// UrlSerializer.hpp


#pragma once


#include "Serializer.hpp"

#include "SNull.hpp"
#include "SBool.hpp"
#include "SStr.hpp"
#include "SBinary.hpp"
#include "SNum.hpp"
#include "SArr.hpp"
#include "SObj.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"

#include <sstream>

class UrlSerializer : public Serializer
{
private:
	std::istringstream _iss;
	std::ostringstream _oss;

	void emplace(SObj* obj, std::string& keyline, const std::string& val);

	SVal* decodeValue(const std::string& strval);

	void encodeNull(const std::string& keyline, const SNull* value);
	void encodeBool(const std::string& keyline, const SBool* value);

	void encodeString(const std::string& keyline, const SStr* value);
	void encodeBinary(const std::string& keyline, const SBinary* value);

	void encodeInteger(const std::string& keyline, const SInt* value);
	void encodeFloat(const std::string& keyline, const SFloat* value);
	void encodeNumber(const std::string& keyline, const SNum* value);

	void encodeArray(const std::string& keyline, const SArr* value);
	void encodeObject(const std::string& keyline, const SObj* value);

	void encodeValue(const std::string& keyline, const SVal* value);

public:
	virtual SVal* decode(const std::string &data, bool strict = false) override;

	virtual std::string encode(const SVal* value) override;
};
