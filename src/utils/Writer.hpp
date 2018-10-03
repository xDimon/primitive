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
// File created on: 2017.03.26

// Writer.hpp


#pragma once

#include <cstddef>
#include <string>

class Writer
{
public:
	virtual char * spacePtr() const = 0;
	virtual size_t spaceLen() const = 0;

	virtual bool prepare(size_t length) = 0;
	virtual bool forward(size_t length) = 0;
	virtual bool write(const void *data, size_t length) = 0;

	inline bool write(const std::string& str)
	{
		return write(str.c_str(), str.length());
	}
};
