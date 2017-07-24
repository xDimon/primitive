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
// File created on: 2017.06.06

// Request.hpp


#pragma once


#include <string>
#include <ucontext.h>
#include <sstream>
#include "ClientTransport.hpp"
#include "../net/TcpConnector.hpp"
#include "../net/ConnectionManager.hpp"
#include "../thread/Task.hpp"
#include "HttpClient.hpp"
#include "http/HttpContext.hpp"
#include "../net/SslConnector.hpp"
#include "../utils/SslHelper.hpp"

class Request: public Task
{
private:
	std::shared_ptr<ClientTransport> _clientTransport;
	HttpRequest::Method _method;
	HttpUri _uri;
	std::string _body;
	std::string _answer;
	std::string _error;

	enum class State
	{
		INIT,
		CONNECT_IN_PROGRESS,
		CONNECTED,
		WRITE,
		READ,
		DONE,
		ERROR
	} _state;

protected:
	virtual bool execute();

public:
	Request(
		const HttpUri& uri,
		HttpRequest::Method method = HttpRequest::Method::GET,
		const std::string& body = std::string()
	);

	~Request() override = default;

	bool operator()()
	{
		_error.clear();
		return execute();
	}

	const std::string& answer() const
	{
		return _answer;
	}
	const std::string& error() const
	{
		return _error;
	}
	const bool hasFailed() const
	{
		return !_error.empty();
	}

	bool connect();
};
