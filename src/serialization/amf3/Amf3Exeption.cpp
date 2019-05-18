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

// Amf3Exeption.cpp


#include "Amf3Exeption.hpp"

Amf3Exeption::Amf3Exeption(std::string msg, size_t pos, std::istream& is)
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

Amf3Exeption::Amf3Exeption(std::string msg, size_t pos)
: _msg(std::move(msg))
{
	_msg += " at position " + std::to_string(pos);
}

Amf3Exeption::Amf3Exeption(std::string msg)
: _msg(std::move(msg))
{
}
