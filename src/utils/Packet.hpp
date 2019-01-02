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
// File created on: 2017.03.27

// Packet.hpp


#pragma once

#include <vector>
#include <string.h>

class Packet
{
private:
	std::vector<char> _data;

public:
	Packet(const char *data, size_t length)
	: _data()
	{
		_data.resize(length);
		memcpy(_data.data(), data, length);
	}
	virtual ~Packet() = default;

	inline const char *data() const
	{
		return _data.data();
	}
	inline size_t size() const
	{
		return _data.size();
	}
};
