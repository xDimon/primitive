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
// File created on: 2017.03.31

// WsFrame.hpp


#pragma once

#include "../../log/Log.hpp"
#include "../../utils/Buffer.hpp"
#include "../../utils/Writer.hpp"

#include <memory>

class WsFrame final : public Buffer
{
public:
	enum class Opcode {
		Continue	= 0x00, // фрейм-продолжение для фрагментированного сообщения. Он интерпретируется, исходя из ближайшего предыдущего ненулевого типа.
		Text 		= 0x01, // текстовый фрейм.
		Binary		= 0x02, // двоичный фрейм.
		_data_0x03	= 0x03,	// зарезервирован для будущих фреймов с данными.
		_data_0x04	= 0x04,	// зарезервирован для будущих фреймов с данными.
		_data_0x05	= 0x05,	// зарезервирован для будущих фреймов с данными.
		_data_0x06	= 0x06,	// зарезервирован для будущих фреймов с данными.
		_data_0x07	= 0x07,	// зарезервирован для будущих фреймов с данными.
		Close		= 0x08,	// закрытие соединения этим фреймом.
		Ping		= 0x09, // PING.
		Pong		= 0x0A, // PONG.
		_crtl_0x0B	= 0x0B, // зарезервирован для будущих управляющих фреймов.
		_crtl_0x0C	= 0x0C, // зарезервирован для будущих управляющих фреймов.
		_crtl_0x0D	= 0x0D, // зарезервирован для будущих управляющих фреймов.
		_crtl_0x0E	= 0x0E, // зарезервирован для будущих управляющих фреймов.
		_crtl_0x0F	= 0x0F, // зарезервирован для будущих управляющих фреймов.
	};

protected:
	bool _finaly;
	Opcode _opcode;
	bool _masked;
	size_t _length;
	uint8_t _mask[4];

public:
	WsFrame(const char *begin, const char *end);
	~WsFrame() override = default;


	bool finaly() const
	{
		return _finaly;
	}
	Opcode opcode() const
	{
		return _opcode;
	}
	size_t contentLength() const
	{
		return _length;
	}

	void applyMask();

	static size_t calcHeaderSize(const char data[2]);

	static void send(const std::shared_ptr<Writer>&, Opcode code, const char* data, size_t size);
};
