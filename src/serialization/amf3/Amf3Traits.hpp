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

// Amf3Traits.hpp


#pragma once

#include <string>
#include <set>
#include <algorithm>
#include <vector>

class Amf3Traits final
{
public:
	std::string className;
	bool dynamic = false;
	bool externalizable = false;

	Amf3Traits() = default; // Default-constructor
	Amf3Traits(Amf3Traits&&) noexcept = delete; // Move-constructor
	Amf3Traits(const Amf3Traits&) = default; // Copy-constructor
	~Amf3Traits() = default; // Destructor
	Amf3Traits& operator=(Amf3Traits&&) noexcept = delete; // Move-assignment
	Amf3Traits& operator=(const Amf3Traits&) = default; // Copy-assignment

	bool operator==(const Amf3Traits& other) const
	{
		return
			dynamic == other.dynamic &&
			externalizable == other.externalizable &&
			className == other.className &&
			attributes == other.attributes;
	}

	bool operator!=(const Amf3Traits& other) const
	{
		return !(*this == other);
	}

	void addAttribute(std::string name)
	{
		if (!hasAttribute(name))
		{
			attributes.push_back(name);
		}
	}

	bool hasAttribute(std::string name) const
	{
		auto it = std::find(attributes.begin(), attributes.end(), name);
		return (it != attributes.end());
	}

	std::set<std::string> getUniqueAttributes() const
	{
		return std::set<std::string>(attributes.begin(), attributes.end());
	}

	// Attribute names
	// Technically, this should be a set. However, since AMF does not actually
	// enforce the sealed property names to be unique, this needs to be a
	// vector to ensure we read the corresponding values, even if they are
	// later overwritten by another value for the same attribute name.
	// Additionally, since traits also can be sent by reference, we need to
	// actually store these duplicate names permanently, in case a later object
	// references a traits object with duplicate attribute names.
	// XXX: figure out whether this interferes with merging
	//      {de,}serializationcontext into one object.
	std::vector<std::string> attributes;

};
