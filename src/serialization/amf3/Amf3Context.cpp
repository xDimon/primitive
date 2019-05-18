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

// Amf3Context.cpp


#include "Amf3Context.hpp"
#include "Amf3Exeption.hpp"

Amf3Context::basic_nullbuf Amf3Context::nullbuf;

std::iostream Amf3Context::nullstream(&Amf3Context::nullbuf);

const std::string& Amf3Context::getString(size_t index)
{
	if (index >= stringReferencesTable.size())
	{
		throw Amf3Exeption("Not found string reference with index=" + std::to_string(index));
	}
	return stringReferencesTable.at(index);
}
int32_t Amf3Context::putSring(const std::string& string)
{
	stringReferencesTable.emplace_back(string);
	auto index = stringReferencesTable.size()-1;
	stringToIndex[string] = index;
	return index;
}
int32_t Amf3Context::getIndex(const std::string& string)
{
	auto it = stringToIndex.find(string);
	return (it != stringToIndex.end()) ? it->second : -1;
}

const Amf3Traits& Amf3Context::getTraits(size_t index)
{
	if (index >= traitsReferencesTable.size())
	{
		throw Amf3Exeption("Not found traits reference with index=" + std::to_string(index));
	}
	return traitsReferencesTable.at(index);
}
int32_t Amf3Context::putTraits(const Amf3Traits& traits)
{
	traitsReferencesTable.push_back(traits);
	return traitsReferencesTable.size() - 1;
}
int32_t Amf3Context::getIndex(const Amf3Traits& traits)
{
	auto it = std::find(traitsReferencesTable.begin(), traitsReferencesTable.end(), traits);
	return (it != traitsReferencesTable.end()) ? it - traitsReferencesTable.begin() : -1;
}
