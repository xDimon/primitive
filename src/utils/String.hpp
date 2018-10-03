// Copyright © 2017-2018 Dmitriy Khaustov
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
// File created on: 2017.05.08

// String.hpp


#pragma once

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

namespace String
{
	size_t utf8strlen(const char *s);

	// trim from start (in place)
	inline void ltrim(std::string &s)
	{
	    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
	            std::not1(std::ptr_fun<int, int>(std::isspace))));
	}

	// trim from end (in place)
	inline void rtrim(std::string &s)
	{
	    s.erase(std::find_if(s.rbegin(), s.rend(),
	            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	}

	// trim from both ends (in place)
	inline void trim(std::string &s)
	{
	    ltrim(s);
	    rtrim(s);
	}

	std::vector<std::string> split(const std::string& in, char separator = ' ');
}
