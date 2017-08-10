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
// File created on: 2017.03.31

// WsFrame.cpp


#include <cstring>
#include "WsFrame.hpp"

#include "../../utils/literals.hpp"

size_t WsFrame::calcHeaderSize(const char data[2])
{
	uint8_t prelen = (static_cast<uint8_t>(data[1]) >> 0) & 0x7F_u8;
	uint8_t masked = (static_cast<uint8_t>(data[1]) >> 7) & 0x01_u8;

	return 2												// Стартовые байты
		+ ((prelen <= 125) ? 0 : (prelen == 126) ? 2 : 8)	// Байты расширенной длины
		+ ((masked == 0) ? 0 : 4);								// Байты маски
}


WsFrame::WsFrame(const char *begin, const char *end)
{
	auto s = begin;

	_finaly = static_cast<bool>((s[0] >> 7) & 0x01);
	_opcode = static_cast<Opcode>((s[0] >> 0) & 0x0F);
	_masked = static_cast<bool>((s[1] >> 7) & 0x01);

	uint8_t prelen = (static_cast<uint8_t>(s[1]) >> 0) & 0x7F_u8;
	if ((s + 2) > end)
	{
		throw std::runtime_error("Not anough data");
	}
	s += 2;

	if (prelen < 126)
	{
		_length = prelen;
	}
	else if (prelen == 126)
	{
		if ((s + 2) > end)
		{
			throw std::runtime_error("Not anough data");
		}
		uint16_t length;
		memcpy(&length, s, sizeof(length));
		s += sizeof(length);
		_length = be16toh(length);
	}
	else if (prelen == 127)
	{
		if ((s + 8) > end)
		{
			throw std::runtime_error("Not anough data");
		}
		uint64_t length;
		memcpy(&length, s, sizeof(length));
		s += sizeof(length);
		_length = be64toh(length);
	}

	if (_masked)
	{
		if ((s + 4) > end)
		{
			throw std::runtime_error("Not anough data");
		}
		_mask[0] = static_cast<uint8_t>(*s++);
		_mask[1] = static_cast<uint8_t>(*s++);
		_mask[2] = static_cast<uint8_t>(*s++);
		_mask[3] = static_cast<uint8_t>(*s);
	}
	else
	{
		_mask[0] =
		_mask[1] =
		_mask[2] =
		_mask[3] = 0;
	}
}

void WsFrame::applyMask()
{
	if (!_masked)
	{
		return;
	}
	int i = 0;
	size_t remain = dataLen();
	for (auto &b : _data)
	{
		b ^= _mask[i++];
		if (--remain == 0)
		{
			break;
		}
		if (i == 4)
		{
			i = 0;
		}
	}
}

void WsFrame::send(const std::shared_ptr<Writer>& writer, Opcode code, const char* data, size_t size)
{
//	bool masked = false;
	uint8_t byte;

	byte = (1_u8 << 7) | static_cast<uint8_t>(code);
	writer->write(&byte, sizeof(byte));

	byte = static_cast<uint8_t>((size > 65535) ? 127 : (size > 125) ? 126 : size);
//	if (masked)
//	{
//		byte |= (1_u8 << 7);
//	}
	writer->write(&byte, sizeof(byte));

	if (size > 65535)
	{
		uint64_t size64 = htobe64(static_cast<uint64_t>(size));
		writer->write(&size64, sizeof(size64));
	}
	else if (size > 125)
	{
		uint16_t size16 = htobe16(static_cast<uint16_t>(size));
		writer->write(&size16, sizeof(size16));
	}

//	if (!masked)
//	{
		writer->write(data, size);
//		return;
//	}
//
//	union {
//		uint32_t i;
//		uint8_t b[4];
//	} mask;
//
//	mask.i = static_cast<uint32_t>(std::rand());
//
//	for (size_t i = 0; i < sizeof(mask.b); i++)
//	{
//		writer->write(&mask.b[i], 1);
//	}
//
//	int i = 0;
//	while (size--)
//	{
//		byte = static_cast<uint8_t>(*data++) ^ mask.b[i++];
//
//		writer->write(&byte, sizeof(byte));
//
//		if (i == 4)
//		{
//			i = 0;
//		}
//	}
}
