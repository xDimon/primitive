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

// HttpResponse.cpp


#include <sstream>
#include <string.h>
#include "HttpResponse.hpp"
#include "../../server/Server.hpp"
#include "../../utils/Time.hpp"

HttpResponse::HttpResponse(int status, std::string message)
: _statusCode(status)
, _statusMessage(std::move(message))
, _close(false)
{
	if (_statusCode < 100 || _statusCode > 599)
	{
		_statusCode = 500;
		_statusMessage = "Internal server error: bad response code";
	}

	if (_statusMessage.empty())
	{
		switch(_statusCode)
		{
			case 100: _statusMessage = "Continue"; break;
			case 101: _statusMessage = "Switching Protocols"; break;
			case 102: _statusMessage = "Processing"; break;
			case 200: _statusMessage = "OK"; break;
			case 201: _statusMessage = "Created"; break;
			case 202: _statusMessage = "Accepted"; break;
			case 203: _statusMessage = "Non-Authoritative Information"; break;
			case 204: _statusMessage = "No Content"; break;
			case 205: _statusMessage = "Reset Content"; break;
			case 206: _statusMessage = "Partial Content"; break;
			case 207: _statusMessage = "Multi-Status"; break;
			case 226: _statusMessage = "IM Used"; break;
			case 300: _statusMessage = "Multiple Choices"; break;
			case 301: _statusMessage = "Moved Permanently"; break;
			case 302: _statusMessage = "Moved Temporarily"; break;
			case 303: _statusMessage = "See Other"; break;
			case 304: _statusMessage = "Not Modified"; break;
			case 305: _statusMessage = "Use Proxy"; break;
			case 307: _statusMessage = "Temporary Redirect"; break;
			case 400: _statusMessage = "Bad Request"; break;
			case 401: _statusMessage = "Unauthorized"; break;
			case 402: _statusMessage = "Payment Required"; break;
			case 403: _statusMessage = "Forbidden"; break;
			case 404: _statusMessage = "Not Found"; break;
			case 405: _statusMessage = "Method Not Allowed"; break;
			case 406: _statusMessage = "Not Acceptable"; break;
			case 407: _statusMessage = "Proxy Authentication Required"; break;
			case 408: _statusMessage = "Request Timeout"; break;
			case 409: _statusMessage = "Conflict"; break;
			case 410: _statusMessage = "Gone"; break;
			case 411: _statusMessage = "Length Required"; break;
			case 412: _statusMessage = "Precondition Failed"; break;
			case 413: _statusMessage = "Request Entity Too Large"; break;
			case 414: _statusMessage = "Request-URI Too Large"; break;
			case 415: _statusMessage = "Unsupported Media Type"; break;
			case 416: _statusMessage = "Requested Range Not Satisfiable"; break;
			case 417: _statusMessage = "Expectation Failed"; break;
			case 422: _statusMessage = "Unprocessable Entity"; break;
			case 423: _statusMessage = "Locked"; break;
			case 424: _statusMessage = "Failed Dependency"; break;
			case 425: _statusMessage = "Unordered Collection"; break;
			case 426: _statusMessage = "Upgrade Required"; break;
			case 428: _statusMessage = "Precondition Required"; break;
			case 429: _statusMessage = "Too Many Requests"; break;
			case 431: _statusMessage = "Request Header Fields Too Large"; break;
			case 449: _statusMessage = "Retry With"; break;
			case 451: _statusMessage = "Unavailable For Legal Reasons"; break;
			case 500: _statusMessage = "Internal Server Error"; break;
			case 501: _statusMessage = "Not Implemented"; break;
			case 502: _statusMessage = "Bad Gateway"; break;
			case 503: _statusMessage = "Service Unavailable"; break;
			case 504: _statusMessage = "Gateway Timeout"; break;
			case 505: _statusMessage = "HTTP Version Not Supported"; break;
			case 506: _statusMessage = "Variant Also Negotiates"; break;
			case 507: _statusMessage = "Insufficient Storage"; break;
			case 508: _statusMessage = "Loop Detected"; break;
			case 509: _statusMessage = "Bandwidth Limit Exceeded"; break;
			case 510: _statusMessage = "Not Extended"; break;
			case 511: _statusMessage = "Network Authentication Required"; break;
			case 520: _statusMessage = "Web server is returning an unknown error"; break;
			case 521: _statusMessage = "Web server is down"; break;
			case 522: _statusMessage = "Connection timed out"; break;
			case 523: _statusMessage = "Origin is unreachable"; break;
			case 524: _statusMessage = "A timeout occurred"; break;
			case 525: _statusMessage = "SSL handshake failed"; break;
			case 526: _statusMessage = "Invalid SSL certificate"; break;
			default:
				if (_statusCode >= 500) _statusMessage = "Server Error";
				else if (_statusCode >= 400) _statusMessage = "Client Error";
				else if (_statusCode >= 300) _statusMessage = "Redirection";
				else if (_statusCode >= 200) _statusMessage = "Success";
				else if (_statusCode >= 100) _statusMessage = "Info";
		}
	}

	addHeader("Connection", "Keep-Alive");
}

HttpResponse::~HttpResponse()
{
}

HttpResponse& HttpResponse::addHeader(const std::string& name, const std::string& value, bool replace)
{
	if (replace)
	{
		removeHeader(name);
	}
	_headers.emplace(name, value);
	return *this;
}

HttpResponse& HttpResponse::removeHeader(const std::string& name)
{
	_headers.erase(name);
	return *this;
}

void HttpResponse::operator>>(TcpConnection& connection)
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n"
		<< "Server: " << Server::httpName() << "\r\n"
		<< "Date: " << Time::httpDate() << "\r\n"
		<< "X-Transport: http\r\n"
		<< "Content-Length: " << _body.str().size() << "\r\n";

	connection.write(oss.str().data(), oss.str().size());

	for (const auto& i : _headers)
	{
		connection.write(i.first.data(), i.first.size());
		connection.write(": ", 2);
		connection.write(i.second.data(), i.second.size());
		connection.write("\r\n", 2);
	}

	connection.write("\r\n", 2);

	connection.write(_body.str().data(), _body.str().size());

	if (_close)
	{
		connection.close();
	}
}

HttpResponse& HttpResponse::operator<<(const HttpHeader& header)
{
	bool replace = header.unique;
	if (header.name == "Connection")
	{
		replace = true;
		if (!strcasecmp(header.value.c_str(), "Close"))
		{
			_close = true;
		}
	}
	addHeader(header.name, header.value, replace);
	return *this;
}