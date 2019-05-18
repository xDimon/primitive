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

// Amf3Serializer.hpp


#pragma once

#include <set>
#include <iostream>
#include "../Serializer.hpp"
#include "Amf3Context.hpp"

class Amf3Serializer final: public Serializer
{
public:
	static const uint32_t HEADLESS = 1u<<31;

private:
	void decodeVersion(Amf3Context& ctx);
	void decodeHeaders(Amf3Context& ctx);
	void decodeMessages(Amf3Context& ctx);

	uint32_t decodeHeadlessInteger(Amf3Context& ctx);
	uint32_t decodeUtf8Symbol(Amf3Context& ctx);
	std::string decodeHeadlessString(Amf3Context& ctx);

	size_t putUtf8Symbol(std::string &str, uint32_t symbol);

	SVal decodeUndefined(Amf3Context& ctx);
	SVal decodeNull(Amf3Context& ctx);
	SVal decodeBool(Amf3Context& ctx);

	SVal decodeString(Amf3Context& ctx);
	SVal decodeBinary(Amf3Context& ctx);

	SVal decodeInteger(Amf3Context& ctx);
	SVal decodeFloat(Amf3Context& ctx);

	SVal decodeArray(Amf3Context& ctx);
	SVal decodeObject(Amf3Context& ctx);
	SVal decodeDictionary(Amf3Context& ctx);

	SVal decodeValue(Amf3Context& ctx);

	void encodeUndefined(Amf3Context& ctx, const SVal& value);
	void encodeNull(Amf3Context& ctx, const SVal& value);
	void encodeBool(Amf3Context& ctx, const SVal& value);

	void encodeString(Amf3Context& ctx, const SVal& value);
	void encodeBinary(Amf3Context& ctx, const SVal& value);

	void encodeHeadlessInteger(Amf3Context& ctx, int64_t value);
	void encodeHeadlessFloat(Amf3Context& ctx, double value);
	void encodeNumber(Amf3Context& ctx, const SVal& value);

	void encodeHeadlessString(Amf3Context& ctx, const std::string& key);

	void encodeArray(Amf3Context& ctx, const SVal& value);
	void encodeObject(Amf3Context& ctx, const SVal& value);
	void encodeDictionary(Amf3Context& ctx, const SVal& value);

	void encodeValue(Amf3Context& ctx, const SVal& value);

DECLARE_SERIALIZER(Amf3Serializer);
};
