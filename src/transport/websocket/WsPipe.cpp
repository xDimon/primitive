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
// File created on: 2017.10.05

// WsPipe.cpp


#include <cstring>
#include "WsPipe.hpp"
#include "../../net/TcpConnection.hpp"
#include "WsContext.hpp"
#include "../../net/ConnectionManager.hpp"

static uint32_t id4noname = 0;

WsPipe::WsPipe(
	const std::shared_ptr<Handler>& handler
)
: _handler(handler)
{
	_name = "WsPipe[" + std::to_string(++id4noname) + "]";
	_log.setName("WsPipe");
	_log.debug("%s created", name().c_str());
}

WsPipe::~WsPipe()
{
	_log.debug("%s destroyed", name().c_str());
}

bool WsPipe::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	auto context = std::dynamic_pointer_cast<WsContext>(connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context-type for this transport");
	}

	if (!context->established())
	{
		throw std::runtime_error("Incomplete websocket communication");
	}

	int n = 0;

	// Цикл извлечения фреймов
	for (;;)
	{
		// http://learn.javascript.ru/websockets#описание-фрейма

		// Пробуем читать новый фрейм
		if (!context->getFrame())
		{
			// Недостаточно данных для получения размера заголовка фрейма
			if (connection->dataLen() < 2)
			{
				if (connection->dataLen() > 0)
				{
					_log.debug("Not anough data for calc frame header size (%zu < 2)", connection->dataLen());
					connection->setTtl(std::chrono::seconds(60));
				}
				else
				{
					_log.debug("No more data");
					connection->setTtl(std::chrono::seconds(900));
				}
				break;
			}

			// Недостаточно данных для чтения заголовка фрейма
			size_t headerSize = WsFrame::calcHeaderSize(connection->dataPtr());
			if (connection->dataLen() < headerSize)
			{
				_log.debug("Not anough data for get frame header (%zu < %zu)", connection->dataLen(), headerSize);
				connection->setTtl(std::chrono::seconds(60));
				break;
			}

			auto frame = std::make_shared<WsFrame>(connection->dataPtr(), connection->dataPtr() + connection->dataLen());
			_log.debug("Read %zu bytes of frame header. Size of data: %zu bytes", headerSize, frame->contentLength());

			context->setFrame(frame);

			// Пропускаем байты заголовка фрейма
			connection->skip(headerSize);
		}

		// Читаем данные фрейма

		size_t len = std::min(
			context->getFrame()->contentLength(),
			connection->dataLen()
		);

		context->getFrame()->write(connection->dataPtr(), len);

		connection->skip(len);

		if (context->getFrame()->contentLength() > context->getFrame()->dataLen())
		{
			_log.debug("Not anough data for read frame body (%zu < %zu)", context->getFrame()->dataLen(), context->getFrame()->contentLength());
			connection->setTtl(std::chrono::seconds(60));
			break;
		}

		context->getFrame()->applyMask();

		switch (context->getFrame()->opcode())
		{
			case WsFrame::Opcode::Text:
			{
				_log.debug("TEXT-FRAME: \"%s\" %zu bytes", context->getFrame()->dataPtr(), context->getFrame()->dataLen());

				connection->setTtl(std::chrono::seconds(900));

				if (metricRequestCount) metricRequestCount->addValue();
				if (metricAvgRequestPerSec) metricAvgRequestPerSec->addValue();
				auto beginTime = std::chrono::steady_clock::now();

				context->handle();

				auto now = std::chrono::steady_clock::now();
				auto timeSpent = static_cast<double>((now - beginTime).count()) / static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
				if (timeSpent > 0)
				{
					if (metricAvgExecutionTime) metricAvgExecutionTime->addValue(timeSpent, now);
				}

				context->resetFrame();
				n++;
				continue;
			}

			case WsFrame::Opcode::Binary:
			{
				_log.debug("BINARY-FRAME: %zu bytes", context->getFrame()->dataLen());

				connection->setTtl(std::chrono::seconds(900));

				if (metricRequestCount) metricRequestCount->addValue();
				if (metricAvgRequestPerSec) metricAvgRequestPerSec->addValue();
				auto beginTime = std::chrono::steady_clock::now();

				context->handle();

				auto now = std::chrono::steady_clock::now();
				auto timeSpent = static_cast<double>((now - beginTime).count()) / static_cast<double>(std::chrono::steady_clock::duration(std::chrono::seconds(1)).count());
				if (timeSpent > 0)
				{
					if (metricAvgExecutionTime) metricAvgExecutionTime->addValue(timeSpent, now);
				}

				context->resetFrame();
				n++;
				continue;
			}
			case WsFrame::Opcode::Ping:
			{
				_log.debug("PING-FRAME: %zu bytes", context->getFrame()->dataLen());

				connection->setTtl(std::chrono::seconds(900));
				WsFrame::send(connection, WsFrame::Opcode::Pong, context->getFrame()->dataPtr(), context->getFrame()->dataLen());

				context->resetFrame();
				n++;
				continue;
			}
			case WsFrame::Opcode::Close:
			{
				_log.debug("CLOSE-FRAME: %zu bytes", context->getFrame()->dataLen());

				std::string msg("##Bye!\n");
				uint16_t code = htobe16(1000); // Normal Closure
				memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
				connection->setTtl(std::chrono::milliseconds(50));

				connection->close();
				connection->resetContext();
				n++;
				goto end;
			}
			default:
				_log.debug("UNSUPPORTED-FRAME(#%d): %zu bytes", static_cast<int>(context->getFrame()->opcode()), context->getFrame()->dataLen());

				std::string msg("##Unsupported opcode\n");
				uint16_t code = htobe16(1003); // Unsupported data
				memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));
				WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
				connection->setTtl(std::chrono::milliseconds(50));

				connection->close();
				connection->resetContext();
				n++;
				goto end;
		}
	}

	end:

	_log.debug("Processed %d frames, remain %zu bytes", n, connection->dataLen());

	return true;
}

void WsPipe::transmit(
	const std::shared_ptr<Connection>& connection_,
	const char* data, size_t size,
	const std::string& contentType,
	bool close
)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	auto context = std::dynamic_pointer_cast<WsContext>(connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context-type for this transport");
	}

	if (!context->established())
	{
		throw std::runtime_error("Incomplete websocket communication");
	}

	auto opcode = (contentType == "text") ? WsFrame::Opcode::Text : WsFrame::Opcode::Binary;

	WsFrame::send(connection, opcode, data, size);
	if (close)
	{
		std::string msg("##Bye!\n");
		uint16_t code = htobe16(1000); // Normal Closure
		memcpy(const_cast<char*>(msg.data()), &code, sizeof(code));

		WsFrame::send(connection, WsFrame::Opcode::Close, msg.c_str(), msg.length());
		connection->setTtl(std::chrono::milliseconds(50));
		connection->close();
		connection->resetContext();
	}
	ConnectionManager::watch(connection);
}
