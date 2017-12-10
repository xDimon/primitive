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

// HttpRequestExecutor.hpp


#pragma once

#include <sys/ucontext.h>
#include "../../utils/Shareable.hpp"
#include "../../log/Log.hpp"
#include "HttpClient.hpp"
#include "HttpRequest.hpp"
#include "../../net/TcpConnector.hpp"

class HttpRequestExecutor: public Shareable<HttpRequestExecutor>
{
private:
	ucontext_t* _savedCtx;
	Log _log;
	std::shared_ptr<HttpClient> _clientTransport;
	HttpRequest::Method _method;
	HttpUri _uri;
	std::string _body;
	std::string _contentType;
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
		const std::string& contentType = ""
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
	const bool hasFailed() const
	{
		return !_error.empty();
	}

	void badStep(const std::string& msg);
};
