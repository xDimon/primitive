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

// WsContext.cpp


#include <random>
#include "WsContext.hpp"
#include "../../utils/Base64.hpp"
#include "WsPipe.hpp"

const std::string& WsContext::key()
{
	if (_key.empty())
	{
		std::default_random_engine generator(static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
		auto rnd1 = std::generate_canonical<double, std::numeric_limits<double>::digits>(generator);
		auto rnd2 = std::generate_canonical<double, std::numeric_limits<double>::digits>(generator);
		uint64_t buff[2] = {
			static_cast<uint64_t>(rnd1 * std::numeric_limits<uint64_t>::max()),
			static_cast<uint64_t>(rnd2 * std::numeric_limits<uint64_t>::max())
		};
		_key = Base64::encode((char*) buff, sizeof(buff));
	}
	return _key;
}

void WsContext::setEstablished()
{
	auto connection = _connection.lock();
	if (_established || !connection)
	{
		return;
	}
	_established = true;

	_frame.reset();
	_request.reset();
	_response.reset();

	auto transport = std::make_shared<WsPipe>(_handler);

	auto prevTransport = std::dynamic_pointer_cast<ServerTransport>(connection->transport());
	if (prevTransport)
	{
		transport->metricRequestCount = prevTransport->metricRequestCount;
		transport->metricAvgRequestPerSec = prevTransport->metricAvgRequestPerSec;
		transport->metricAvgExecutionTime = prevTransport->metricAvgExecutionTime;
	}

	connection->setTransport(transport);

	_transmitter =
		std::make_shared<Transport::Transmitter>(
			[transport, wp = _connection]
			(const char* data, size_t size, const std::string& contentType, bool close)
			{
				transport->transmit(wp.lock(), data, size, contentType, close);
			}
		);

	transport->processing(connection);

	if (_establishHandler)
	{
		_establishHandler(*this);
	}
}

void WsContext::addEstablishedHandler(std::function<void(WsContext&)> handler)
{
	_establishHandler = std::move(handler);
}
