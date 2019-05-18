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
// File created on: 2017.05.01

// HttpResponse.hpp


#pragma once

#include "HttpHeader.hpp"
#include "../../net/TcpConnection.hpp"

#include <sstream>
#include <map>

class HttpResponse final: public Buffer
{
public:
	enum class TransferEncoding {
		NONE,
		CHUNKED,
		GZIP,
	};

private:
	const enum class Type
	{
		RECEIVED,
		FOR_SENDING
	} _type;

	uint8_t _protocolVersion;
	int _statusCode;
	std::string _statusMessage;

	std::string _mainHeader;
	std::multimap<std::string, std::string> _headers;

	bool _hasContentLength;
	size_t _contentLength;

	uint8_t _transferEncodings;

	std::ostringstream _body;

	bool _close;

	const char* parseResponseLine(const char* string, const char* end);

	const char* parseProtocol(const char* s);

	const char* parseHeaders(const char* begin, const char* end);

public:
	HttpResponse() = delete;
	HttpResponse(const HttpResponse&) = delete;
	HttpResponse& operator=(const HttpResponse&) = delete;
	HttpResponse(HttpResponse&&) noexcept = delete;
	HttpResponse& operator=(HttpResponse&&) noexcept = delete;

	HttpResponse(int status, const std::string& message, uint8_t protocolVersion = 101);
	explicit HttpResponse(int status)
	: HttpResponse(status, "")
	{
	}
	HttpResponse(const char *begin, const char *end);
	~HttpResponse() override = default;

	HttpResponse& addHeader(const std::string& name, const std::string& value, bool replace = false);

	HttpResponse& removeHeader(const std::string& name);

	template<class T>
	HttpResponse& operator<<(const T& chunk)
	{
		_body << chunk;
		return *this;
	}

	HttpResponse& operator<<(const HttpHeader& header);

	void operator>>(TcpConnection& connection);

	std::string& getHeader(std::string name);

	int statusCode() const
	{
		return _statusCode;
	}
	const std::string& statusMessage() const
	{
		return _statusMessage;
	}

	bool hasContentLength() const
	{
		return _hasContentLength;
	}

	size_t contentLength() const
	{
		return _contentLength;
	}

	bool ifTransferEncoding(TransferEncoding transferEncoding);
};
