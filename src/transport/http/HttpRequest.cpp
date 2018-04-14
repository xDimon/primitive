// Copyright © 2017-2018 Dmitriy Khaustov
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

// HttpRequest.cpp


#include "HttpRequest.hpp"
#include "HttpHelper.hpp"
#include "../../utils/String.hpp"

#include <memory.h>
#include <algorithm>
#include <sstream>

HttpRequest::HttpRequest(const char* begin, const char* end)
: _method(HttpRequest::Method::UNKNOWN)
, _protocolVersion(100)
, _hasContentLength(false)
, _contentLength(0)
, _transferEncodings(0)
{
	auto s = begin;

	try
	{
		s =	parseRequestLine(s, end);

		parseHeaders(s, end);
	}
	catch (std::exception& exception)
	{
		throw std::runtime_error(std::string("Bad request ← ") + exception.what());
	}
}

const char* HttpRequest::parseRequestLine(const char *string, const char *end)
{
	auto s = string;

	try
	{
		auto endHeader = static_cast<const char *>(memmem(s, end - s, "\r\n", 2));
		if (endHeader == nullptr)
		{
			throw std::runtime_error("Unexpected end");
		}

		try
		{
			s = parseMethod(s);
		}
		catch (std::exception &exception)
		{
			throw std::runtime_error(std::string("Bad method ← ") + exception.what());
		}

		// Читаем URI
		auto uri = s;

		// Находим конец URI
		while (*s != 0 && !HttpHelper::isSp(*s) && !HttpHelper::isCr(*s)) s++;
		if (isspace(*s) == 0)
		{
			throw std::runtime_error("Bad uri ← Unexpected end");
		}

		try
		{
			_uri.parse(uri, s - uri);
		}
		catch (std::exception &exception)
		{
			throw std::runtime_error(std::string("Bad uri ← ") + exception.what());
		}

		s++;

		// Читаем протокол и версию
		try
		{
			s = parseProtocol(s);
		}
		catch (std::exception &exception)
		{
			throw std::runtime_error(std::string("Bad protocol ← ") + exception.what());
		}

		if (!HttpHelper::isCrlf(s))
		{
			throw std::runtime_error("Redundant data on end of line");
		}
		s += 2;
	}
	catch (std::exception &exception)
	{
		throw std::runtime_error(std::string("Bad request line ← ") + exception.what());
	}

	return s;
}

const char* HttpRequest::parseMethod(const char* s)
{
	// Чистаем метод
	if (strncasecmp(s, "GET", 3) == 0)
	{
		_method = Method::GET;
		s += 3;
	}
	else if (strncasecmp(s, "POST ", 4) == 0)
	{
		_method = Method::POST;
		s += 4;
	}
	else
	{
		throw std::runtime_error("Unknown method");
	}

	if (isspace(*s) == 0)
	{
		throw std::runtime_error("Wrong method");
	}

	s++;

	return s;
}

const char* HttpRequest::parseProtocol(const char* s)
{
	if (strncasecmp(s, "HTTP/", 5) != 0)
	{
		throw std::runtime_error("Wrong type");
	}
	s += 5;

	if (strncasecmp(s, "1.1", 3) == 0)
	{
		_protocolVersion = 101;
	}
	else if (strncasecmp(s, "1.0", 3) == 0)
	{
		_protocolVersion = 100;
	}
	else
	{
		throw std::runtime_error("Wrong version");
	}

	s += 3;

	return s;
}

const char* HttpRequest::parseHeaders(const char *begin, const char *end)
{
	auto s = begin;
	std::string *prevHeaderValue = nullptr;

	try
	{
		for (;;)
		{
			auto endHeader = static_cast<const char *>(memmem(s, end - s, "\r\n", 2));
			if (endHeader == nullptr)
			{
				throw std::runtime_error("Unexpected end");
			}
			// Пустой заголовок - заголовки закончились
			if (endHeader == s)
			{
				s += 2;
				return s;
			}

			// Проверка на возможный многострочный заголовок
			if (prevHeaderValue != nullptr)
			{
				if (HttpHelper::isSp(*s) || HttpHelper::isHt(*s))
				{
					while (HttpHelper::isSp(*s) || HttpHelper::isHt(*s))
					{
						s++;
					}
					prevHeaderValue->push_back(' ');

					while (!HttpHelper::isCrlf(s))
					{
						prevHeaderValue->push_back(*s++);
					}
					s += 2;

					continue;
				}
				else
				{
					prevHeaderValue = nullptr;
				}
			}

			// Имя заголовка
			std::string name;
			while (HttpHelper::isToken(*s))
			{
				name.push_back(*s++);
			}

			// Разделитель
			if (*s != ':')
			{
				throw std::runtime_error("Bad name");
			}
			s++;

			// Значение заголовка
			std::string value;
			while (HttpHelper::isSp(*s) || HttpHelper::isHt(*s))
			{
				s++;
			}
			while (!HttpHelper::isCrlf(s))
			{
				value.push_back(*s++);
			}
			s += 2;

			std::transform(name.begin(), name.end(), name.begin(), ::toupper);

			auto i = _headers.emplace(name, value);

			prevHeaderValue = &i->second;

			if (strcasecmp(name.c_str(), "Content-Length") == 0)
			{
				_hasContentLength = true;
				_contentLength = static_cast<size_t>(atoll(value.c_str()));
			}

			if (strcasecmp(name.c_str(), "Host") == 0)
			{
				_uri.setHost(value);
			}
		}
	}
	catch (std::exception &exception)
	{
		throw std::runtime_error(std::string("Bad headers ← ") + exception.what());
	}
}

std::string HttpRequest::getHeader(std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);

	auto i = _headers.find(name);
	if (i == _headers.end())
	{
		static std::string empty;
		return empty;
	}
	return i->second;
}

bool HttpRequest::ifTransferEncoding(HttpRequest::TransferEncoding transferEncoding)
{
	if (_transferEncodings == 0)
	{
		_transferEncodings |= static_cast<uint8_t>(TransferEncoding::NONE);
		auto header = getHeader("Transfer-Encoding");
		if (!header.empty())
		{
			std::istringstream iss(header);
			std::string s;
			while (std::getline(iss, s, ','))
			{
				if (s == "chunked")
				{
					_transferEncodings |= static_cast<uint8_t>(TransferEncoding::CHUNKED);
				}
				else if (s == "gzip")
				{
					_transferEncodings |= static_cast<uint8_t>(TransferEncoding::GZIP);
				}
			}
		}
	}

	return _transferEncodings & static_cast<uint8_t>(transferEncoding);
}
