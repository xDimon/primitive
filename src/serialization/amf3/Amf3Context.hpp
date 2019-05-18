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
// File created on: 2019.05.14

// Amf3Context.hpp


#pragma once


#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include "Amf3Traits.hpp"

class Amf3Context final
{
	struct basic_nullbuf : std::basic_streambuf<char, std::char_traits<char>> {
		typedef std::basic_streambuf<char, std::char_traits<char>> base_type;
		typedef typename base_type::int_type int_type;
		typedef typename base_type::traits_type traits_type;

		virtual int_type overflow(int_type c) {
			return traits_type::not_eof(c);
		}
	};

	static basic_nullbuf nullbuf;
	static std::iostream nullstream;

	std::vector<std::string> stringReferencesTable;
	std::unordered_map<std::string, size_t> stringToIndex;
	std::vector<Amf3Traits> traitsReferencesTable;

public:
	std::istream& is;
	std::ostream& os;

	Amf3Context() = delete; // Default-constructor
	Amf3Context(Amf3Context&&) noexcept = delete; // Move-constructor
	Amf3Context(const Amf3Context&) = delete; // Copy-constructor
	~Amf3Context() = default; // Destructor
	Amf3Context& operator=(Amf3Context&&) noexcept = delete; // Move-assignment
	Amf3Context& operator=(Amf3Context const&) = delete; // Copy-assignment

	Amf3Context(std::istream& is)
	: is(is)
	, os(nullstream)
	{}

	Amf3Context(std::ostream& os)
	: is(nullstream)
	, os(os)
	{}

	const std::string& getString(size_t index);
	int32_t putSring(const std::string& string);
	int32_t getIndex(const std::string& string);

	const Amf3Traits& getTraits(size_t index);
	int32_t putTraits(const Amf3Traits& traits);
	int32_t getIndex(const Amf3Traits& traits);
};
