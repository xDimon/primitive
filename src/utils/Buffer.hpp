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

// Buffer.hpp


#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

class Buffer
{
public:
	class Chunk
	{
	public:
		char * data;
		size_t length;
		Chunk(char *data, size_t length)
			: data(data)
			, length(length)
		{};
	};

private:
	/// Мютекс защиты буфера ввода
	mutable std::recursive_mutex _mutex;

	/// Вектор, контейнер данных буфера
	std::vector<char> _data;

	/// Смещение на точку, откуда будут извлекаться данные из буфера
	size_t _getPosition;

	/// Смещение на точку, куда будут добавляться данные в буфер
	size_t _putPosition;

public:
	Buffer();
	virtual ~Buffer();

	inline auto& mutex() { return _mutex; }

	const char * dataPtr() const;
	size_t dataLen() const;
//	const Chunk data() const;

	char * spacePtr() const;
	size_t spaceLen() const;
//	const Chunk space() const;

	bool empty() const;
	size_t size() const;

	bool show(char *data, size_t length) const;
	bool skip(size_t length);
	bool read(char *data, size_t length);

	bool prepare(size_t length);
	bool forward(size_t length);
	bool write(const char *data, size_t length);

};
