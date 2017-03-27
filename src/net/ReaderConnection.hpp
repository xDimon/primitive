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

// ReaderConnection.hpp


#pragma once

#include "../utils/Buffer.hpp"
#include "../utils/Reader.hpp"

class ReaderConnection: public Reader
{
protected:
	/// Буфер ввода
	Buffer _inBuff;

public:
	ReaderConnection() {}
	virtual ~ReaderConnection() {}

	virtual inline const char * dataPtr() const
	{
		return _inBuff.dataPtr();
	}
	virtual inline size_t dataLen() const
	{
		return _inBuff.dataLen();
	}

	virtual inline bool show(void *data, size_t length) const
	{
		return _inBuff.show(data, length);
	}
	virtual inline bool skip(size_t length)
	{
		return _inBuff.skip(length);
	}
	virtual inline bool read(void *data, size_t length)
	{
		return _inBuff.read(data, length);
	}
};
