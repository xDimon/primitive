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

// HttpRequest.cpp


#include "HttpRequest.hpp"
#include "HttpHelper.hpp"

#include <memory.h>

HttpRequest::HttpRequest(const char* begin, const char* end)
: _hasContentLength(false)
{
	auto s = begin;

	try
	{
		s =	parseRequestLine(s, end);

		parseHeaders(s, end);
	}
	catch (std::runtime_error& exception)
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
		catch (std::runtime_error &exception)
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
		catch (std::runtime_error &exception)
		{
			throw std::runtime_error(std::string("Bad uri ← ") + exception.what());
		}

		s++;

		// Читаем протокол и версию
		try
		{
			s = parseProtocol(s);
		}
		catch (std::runtime_error &exception)
		{
			throw std::runtime_error(std::string("Bad protocol ← ") + exception.what());
		}

		if (!HttpHelper::isCrlf(s))
		{
			throw std::runtime_error("Redundant data on end of line");
		}
		s += 2;
	}
	catch (std::runtime_error &exception)
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
		_version = 101;
	}
	else if (strncasecmp(s, "1.0", 3) == 0)
	{
		_version = 100;
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

			// Имя заголовка
			std::string name;
			while (HttpHelper::isToken(*s))
			{
				name.push_back(*s++);
			}

			if (*s != ':')
			{
				throw std::runtime_error("Bad name");
			}

			s = HttpHelper::skipLws(++s);

			// Значение заголовка
			std::string value;
			while (!HttpHelper::isCrlf(s))
			{
				value.push_back(*s++);
			}

			s += 2;

			_headers.emplace(name, value);

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
	catch (std::runtime_error &exception)
	{
		throw std::runtime_error(std::string("Bad headers ← ") + exception.what());
	}
}

std::string& HttpRequest::getHeader(std::string name)
{
	auto i = _headers.find(name);
	if (i == _headers.end())
	{
		static std::string empty;
		return empty;
	}
	return i->second;
}
