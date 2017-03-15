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
// File created on: 2017.02.28

// Buffer.cpp


#include "Buffer.hpp"

#include <algorithm>
#include <cstring>
#include "../log/Log.hpp"

Buffer::Buffer()
: _getPosition(0)
, _putPosition(0)
{
}

Buffer::~Buffer()
{
}

const char *Buffer::dataPtr() const
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	return _data.data() + _getPosition;
}

size_t Buffer::dataLen() const
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	return _putPosition - _getPosition;
}

//const Buffer::Chunk Buffer::data() const
//{
//	std::lock_guard<std::recursive_mutex> guard(_mutex);
//	return Chunk(const_cast<char *>(_data.data()) + _getPosition, _putPosition - _getPosition);
//}

char *Buffer::spacePtr() const
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	return const_cast<char *>(_data.data()) + _putPosition;
}

size_t Buffer::spaceLen() const
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	return _data.size() - _putPosition;
}

//const Buffer::Chunk Buffer::space() const
//{
//	std::lock_guard<std::recursive_mutex> guard(_mutex);
//	return Chunk(const_cast<char *>(_data.data()) + _putPosition, _data.size() - _putPosition);
//}

size_t Buffer::size() const
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	return _data.size();
}

bool Buffer::show(char *data, size_t length) const
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	if (_putPosition - _getPosition < length)
	{
		return false;
	}
	memcpy(data, _data.data() + _getPosition, length);
	return true;
}

bool Buffer::skip(size_t length)
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	if (_putPosition - _getPosition < length)
	{
		return false;
	}
	_getPosition += length;
	return true;
}

bool Buffer::read(char *data, size_t length)
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	if (_putPosition - _getPosition < length)
	{
		return false;
	}

	while (length--)
	{
		*data++ = _data[_getPosition++];
	}
	return true;
}

bool Buffer::prepare(size_t length)
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	// Недостаточно места в конце буффера
	if (_putPosition + length > _data.size())
	{
		// Если есть прочитанные данные в начале буффера
		if (_getPosition > 0)
		{

			// Смещаем непрочитанные данные в начало буффера
			std::copy_n(_data.cbegin() + _getPosition, _putPosition - _getPosition, _data.begin());

			_putPosition -= _getPosition;
			_getPosition = 0;
		}
	}
	// Все еще недостаточно места в конце буффера
	if (_putPosition + length > _data.size())
	{
		_data.resize(((_putPosition + length) / (1<<12) + 1) * (1<<12));
	}
	return true;
}

bool Buffer::forward(size_t length)
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	if (_putPosition + length > spaceLen())
	{
		return false;
	}


	_putPosition += length;
	return true;
}

bool Buffer::write(const char *data, size_t length)
{
	if (length == 0)
	{
		return true;
	}
	std::lock_guard<std::recursive_mutex> guard(_mutex);

	prepare(length);


	while (length--)
	{
		_data[_putPosition++] = *data++;
	}
	return true;
}
