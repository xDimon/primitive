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
// File created on: 2017.06.06

// HttpRequestExecutor.hpp


#pragma once

#include <sys/ucontext.h>
#include "../../utils/Shareable.hpp"
#include "../../log/Log.hpp"
#include "HttpClient.hpp"
#include "HttpRequest.hpp"
#include "../../net/TcpConnector.hpp"

class HttpRequestExecutor final: public Shareable<HttpRequestExecutor>
{
private:
	ucontext_t* _savedCtx;
	Log _log;
	std::shared_ptr<HttpClient> _clientTransport;
	HttpRequest::Method _method;
	HttpUri _uri;
	std::string _body;
	std::string _contentType;
	std::chrono::milliseconds _timeout;
	std::string _answer;
	std::string _error;
	std::recursive_mutex _mutex;
	std::shared_ptr<TcpConnector> _connector;
	std::shared_ptr<TcpConnection> _connection;

	enum class State
	{
		INIT 		= 0,
		CONNECT		= 1,
		CONNECTED	= 2,
		SUBMIT		= 3,
		SUBMITED	= 4,
		COMPLETE	= 5,
		ERROR		= 255
	} _state;

	static std::string stateToString(State state)
	{
		switch (state)
		{
			case State::INIT:       { static const std::string s("INIT");       return s;}
			case State::CONNECT:    { static const std::string s("CONNECT");    return s;}
			case State::CONNECTED:  { static const std::string s("CONNECTED");  return s;}
			case State::SUBMIT:     { static const std::string s("SUBMIT");     return s;}
			case State::SUBMITED:   { static const std::string s("SUBMITED");   return s;}
			case State::COMPLETE:   { static const std::string s("COMPLETE");   return s;}
			case State::ERROR:      { static const std::string s("ERROR");      return s;}
			default:                { static const std::string s("UNKNOWN");    return s; }
		}
	}

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
	HttpRequestExecutor(
		const HttpUri& uri,
		HttpRequest::Method method = HttpRequest::Method::GET,
		const std::string& body = "",
		const std::string& contentType = "",
		std::chrono::milliseconds timeout = std::chrono::seconds(60)
	);

	~HttpRequestExecutor() override;

	void operator()();

	const std::string& uri() const
	{
		return _uri.str();
	}
	const std::string& answer() const
	{
		return _answer;
	}
	const std::string& error() const
	{
		return _error;
	}
	bool hasFailed() const
	{
		return !_error.empty();
	}

	void badStep(const std::string& msg);
};
