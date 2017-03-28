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
// File created on: 2017.03.26

// HttpRequest.hpp


#pragma once

#include <map>
#include <memory>
#include "../../log/Log.hpp"
#include "../../utils/Buffer.hpp"
#include "HttpUri.hpp"

class HttpRequest: public virtual Log, public Buffer
{
public:
	typedef std::shared_ptr<HttpRequest> Ptr;
	typedef std::weak_ptr<HttpRequest> WPtr;

	enum class Method {
		GET,
		POST,
		UNKNOWN
	};

private:
	Method _method;
	HttpUri _uri;
	uint8_t _version;
	std::multimap<std::string, std::string> _headers;
	bool _hasContentLength;
	size_t _contentLength;

	const char* parseMethod(const char* string);
	const char* parseProtocol(const char* string);
	const char* parseRequestLine(const char *string, const char *end);
	const char* parseHeaders(const char *string, const char *end);

public:
	HttpRequest(const char *begin, const char *end);
	virtual ~HttpRequest() {};

	const HttpUri& uri() const
	{
		return _uri;
	}

	std::string uri_s() const
	{
		return _uri.str();
	}

	Method method() const
	{
		return _method;
	}

	std::string method_s() const
	{
		return
			_method == Method::GET ? "GET" :
			_method == Method::POST ? "POST" :
			"*method*";
	}

	uint8_t protocol() const
	{
		return _version;
	}

	std::string protocol_s() const
	{
		return
			_version == 100 ? "HTTP/1.0" :
			_version == 101 ? "HTTP/1.1" :
			"*protocol*";
	}

	bool hasContentLength() const
	{
		return _hasContentLength;
	}

	size_t contentLength() const
	{
		return _contentLength;
	}

	std::string& getHeader(std::string name);
};
