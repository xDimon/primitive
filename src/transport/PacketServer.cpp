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
// File created on: 2017.03.10

// PacketServer.cpp


#include <sstream>
#include "PacketServer.hpp"
#include "../net/TcpConnection.hpp"
#include "../utils/Packet.hpp"

REGISTER_TRANSPORT(packet, PacketServer);

bool PacketServer::processing(const std::shared_ptr<Connection>& connection_)
{
	auto connection = std::dynamic_pointer_cast<TcpConnection>(connection_);
	if (!connection)
	{
		throw std::runtime_error("Bad connection-type for this transport");
	}

	int n = 0;

	// Цикл извлечения пакетов
	for (;;)
	{
		uint16_t packetSize;

		// Недостаточно данных для чтения размера пакета
		if (!connection->show(&packetSize, sizeof(packetSize)))
		{
			if (connection->dataLen())
			{
				_log.debug("Not anough data for read packet size (%zu < 2)", connection->dataLen());
			}
			else
			{
				_log.debug("No more data");
			}
			break;
		}
		_log.debug("Read 2 bytes of packet size: %hu bytes", packetSize);

		// Недостаточно данных для формирования пакета (не все данные получены)
		if (connection->dataLen() < sizeof(packetSize) + packetSize)
		{
			_log.debug("Not anough data for read packet body (%zu < %hu)", connection->dataLen() - sizeof(packetSize), packetSize);
			break;
		}
		_log.debug("Read %hu bytes of packet body", packetSize);

		// Пропускаем байты размера пакета
		connection->skip(sizeof(packetSize));

		// Читаем пакет
		Packet request(connection->dataPtr(), packetSize);

		// Пропускаем байты тела пакета
		connection->skip(packetSize);

		std::stringstream ss;
		ss << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Processed packet (len=" << packetSize << ")\r\n";

		// Формируем пакет
		Packet response(ss.str().c_str(), ss.str().size());

		// Отправляем
		connection->write(response.data(), response.size());

		n++;
	}

	_log.debug("Processed %d packets, remain %zu bytes", n, connection->dataLen());

	return false;
}

void PacketServer::bindHandler(const std::string& selector, const std::shared_ptr<ServerTransport::Handler>& handler)
{
	// TODO реализовать
	throw std::runtime_error("Not implemented");
}

std::shared_ptr<ServerTransport::Handler> PacketServer::getHandler(const std::string& subject)
{
	// TODO реализовать
	throw std::runtime_error("Not implemented");
}
