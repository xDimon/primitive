// Copyright © 2017 Dmitriy Khaustov
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

// WriterConnection.hpp


#pragma once

#include "../utils/Buffer.hpp"
#include "../utils/Writer.hpp"

class WriterConnection : public Writer
{
protected:
	Buffer _outBuff;

public:
	inline char* spacePtr() const override
	{
		return _outBuff.spacePtr();
	}
	inline size_t spaceLen() const override
	{
		return _outBuff.spaceLen();
	}

	inline bool prepare(size_t length) override
	{
		return _outBuff.prepare(length);
	}
	inline bool forward(size_t length) override
	{
		return _outBuff.forward(length);
	}
	inline bool write(const void* data, size_t length) override
	{
		return _outBuff.write(data, length);
	}
};
