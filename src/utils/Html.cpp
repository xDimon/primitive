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
// File created on: 2017.07.03

// Html.cpp


#include "Html.hpp"


#include <sstream>

namespace Html
{

std::string escape(const std::string& string)
{
	std::istringstream iss(string);

	std::string ret;

	while (!iss.eof())
	{
		auto c = iss.get();
		if (c == -1)
		{
			break;
		}
		switch (c)
		{
			case '&': ret += "&amp;";
				break;

			case '<': ret += "&lt;";
				break;

			case '>': ret += "&gt;";
				break;

			case '"': ret += "&quot;";
				break;

			case '\'': ret += "&apos;";
				break;

			default: ret.push_back(static_cast<uint8_t>(c));
		}
	}

	return std::move(ret);
}

}
