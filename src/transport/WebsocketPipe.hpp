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

// WebsocketPipe.hpp


#pragma once


#include "Transport.hpp"
#include "../net/TcpConnection.hpp"

class WebsocketPipe : public Transport
{
private:
	std::shared_ptr<Handler> _handler;

public:
	WebsocketPipe(const WebsocketPipe&) = delete; // Copy-constructor
	void operator=(WebsocketPipe const&) = delete; // Copy-assignment
	WebsocketPipe(WebsocketPipe&&) = delete; // Move-constructor
	WebsocketPipe& operator=(WebsocketPipe&&) = delete; // Move-assignment

	WebsocketPipe(
		const std::shared_ptr<Handler>& handler
	);

	~WebsocketPipe() override;

	bool processing(const std::shared_ptr<Connection>& connection) override;

	void transmit(
		const std::shared_ptr<Connection>& connection,
		const char* data, size_t size,
		const std::string& contentType,
		bool close
	);
};
