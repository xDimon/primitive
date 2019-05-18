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

class UrlSerializer final: public Serializer
{
	void emplace(SObj& obj, std::string& keyline, const std::string& val);

	SVal decodeValue(const std::string& strval);

	void encodeNull(std::ostream& os, const std::string& keyline, const SVal& value);
	void encodeBool(std::ostream& os, const std::string& keyline, const SVal& value);

	void encodeString(std::ostream& os, const std::string& keyline, const SVal& value);
	void encodeBinary(std::ostream& os, const std::string& keyline, const SVal& value);

	void encodeNumber(std::ostream& os, const std::string& keyline, const SVal& value);

	void encodeArray(std::ostream& os, const std::string& keyline, const SVal& value);
	void encodeObject(std::ostream& os, const std::string& keyline, const SVal& value);

	void encodeValue(std::ostream& os, const std::string& keyline, const SVal& value);

DECLARE_SERIALIZER(UrlSerializer);
};
