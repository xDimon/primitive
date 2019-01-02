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
// File created on: 2017.02.28

// Buffer.hpp


#pragma once

#include "Reader.hpp"
#include "Writer.hpp"

#include <cstddef>
#include <mutex>
#include <vector>

class Buffer : public Reader, public Writer
{
protected:
	/// Мютекс защиты буфера ввода
	mutable std::recursive_mutex _mutex;

	/// Вектор, контейнер данных буфера
	std::vector<char> _data;

	/// Смещение на точку, откуда будут извлекаться данные из буфера
	size_t _getPosition;

	/// Смещение на точку, куда будут добавляться данные в буфер
	size_t _putPosition;

public:
	Buffer(const Buffer&) = delete;
	Buffer& operator=(Buffer const&) = delete;
	Buffer(Buffer&&) noexcept = delete;
	Buffer& operator=(Buffer&&) noexcept = delete;

	Buffer();
	virtual ~Buffer() = default;

	inline auto& mutex()
	{ return _mutex; }

	size_t size() const;

	virtual const std::vector<char>& data();
	const char* dataPtr() const override;
	size_t dataLen() const override;

	bool show(void* data, size_t length) const override;
	bool skip(size_t length) override;
	bool read(void* data, size_t length) override;

//	virtual std::vector<char>& space() const;
	char* spacePtr() const override;
	size_t spaceLen() const override;

	bool prepare(size_t length) override;
	bool forward(size_t length) override;
	bool write(const void* data, size_t length) override;
};
