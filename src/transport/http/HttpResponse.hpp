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
// File created on: 2017.05.01

// HttpResponse.hpp


#pragma once

#include <string>
#include <map>
#include "../../net/WriterConnection.hpp"
#include "../../net/TcpConnection.hpp"
#include "HttpHeader.hpp"

class HttpResponse
{
private:
	int _statusCode;
	std::string _statusMessage;

	std::string _mainHeader;
	std::multimap<std::string, std::string> _headers;

	std::ostringstream _body;

	bool _close;

public:
	HttpResponse(int status, std::string message = std::string());

	~HttpResponse();

	HttpResponse& addHeader(const std::string& name, const std::string& value, bool replace = false);

	HttpResponse& removeHeader(const std::string& name);

	HttpResponse& close();

	template<class T>
	HttpResponse& operator<<(const T& chunk)
	{
		_body << chunk;
		return *this;
	}

	HttpResponse& operator<<(const HttpHeader& header);

	void operator>>(TcpConnection& connection);
};
