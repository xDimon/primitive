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
// File created on: 2017.10.07

// WsCommunicator.hpp


#pragma once

#include <sys/ucontext.h>
#include <mutex>
#include "../../utils/Shareable.hpp"
#include "../../log/Log.hpp"
#include "WsClient.hpp"
#include "transport/URI.hpp"
#include "../../net/TcpConnector.hpp"

class WsCommunicator: public Shareable<WsCommunicator>
{
private:
	ucontext_t* _savedCtx;
	Log _log;
	std::shared_ptr<WsClient> _websocketClient;
	URI _uri;
	std::string _error;
	std::recursive_mutex _mutex;
	std::shared_ptr<TcpConnector> _connector;
	std::shared_ptr<TcpConnection> _connection;

	enum class State
	{
		INIT,
		CONNECT,
		CONNECTED,
		SUBMIT,
		SUBMITED,
		ESTABLISHED,
		ERROR
	} _state;

	void onConnected();
	void failConnect();
	void exceptionAtConnect();

	void submit();
	void exceptionAtSubmit();

	void onComplete();
	void failProcessing();
	void onError();

	void done();

public:
	WsCommunicator(
		const URI& uri,
		const std::shared_ptr<Transport::Handler>& handler
	);
	~WsCommunicator() override;

	void operator()();

	const std::string& uri() const
	{
		return _uri.str();
	}
	std::shared_ptr<TcpConnection> connection() const
	{
		return _connection;
	}
	std::shared_ptr<WsClient> client() const
	{
		return _websocketClient;
	}
	const std::string& error() const
	{
		return _error;
	}
	const bool hasFailed() const
	{
		return !_error.empty();
	}

	void badStep(const std::string& msg);
};
