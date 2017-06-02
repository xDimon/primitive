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
// File created on: 2017.03.29

// WsContext.hpp


#pragma once

#include "../../utils/Context.hpp"
#include "../http/HttpRequest.hpp"
#include "../../net/TcpConnection.hpp"
#include "../../transport/Transport.hpp"

#include "WsFrame.hpp"
#include <memory>

class WsContext: public Context
{
private:
	std::shared_ptr<HttpRequest> _request;
	bool _established;
	std::shared_ptr<TcpConnection> _connection;
	std::shared_ptr<WsFrame> _frame;
	Transport::Handler _handler;

public:
	WsContext(std::shared_ptr<TcpConnection> connection)
	: _established(false)
	, _connection(connection)
	{};
	virtual ~WsContext() {};

	void setEstablished()
	{
		_established = true;
		_frame.reset();
	}
	bool established()
	{
		return _established;
	}

	std::shared_ptr<TcpConnection> connection()
	{
		return _connection;
	}

	void setRequest(std::shared_ptr<HttpRequest>& request)
	{
		_request = request;
	}
	std::shared_ptr<HttpRequest>& getRequest()
	{
		return _request;
	}

	void setHandler(Transport::Handler handler)
	{
		_handler = handler;
	}
	void handle(const char*data, size_t size, Transport::Transmitter transmitter)
	{
		_handler(ptr(), data, size, transmitter);
	}

	void setFrame(std::shared_ptr<WsFrame>& frame)
	{
		_frame = frame;
	}
	std::shared_ptr<WsFrame>& getFrame()
	{
		return _frame;
	}
};
