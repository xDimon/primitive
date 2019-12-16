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
// File created on: 2017.03.26

// URI.cpp


#include "URI.hpp"
#include "utils/encoding/PercentEncoding.hpp"

#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

URI::URI()
: _scheme(Scheme::UNDEFINED)
, _secure(false)
, _port(0)
, _hasQuery(false)
, _hasFragment(false)
{
}

URI::URI(std::string_view uri)
: _scheme(Scheme::UNDEFINED)
, _secure(false)
, _port(0)
, _hasQuery(false)
, _hasFragment(false)
{
	try
	{
		parse(uri.data(), uri.length());
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Bad URI ← ") + exception.what());
	}
}

void URI::parse(const char *string, size_t length)
{
	auto s = string;
	auto end = string + length;

	// Читаем схему
	if (length >= 2 && strncmp(s, "//", 2) == 0)
	{
		_scheme = Scheme::UNDEFINED;
		s += 2;
	}
	else if (*s == '/')
	{
		_scheme = Scheme::UNDEFINED;
		goto path;
	}
	else if (length >= 5 && strncasecmp(s, "ws://", 5) == 0)
	{
		_scheme = Scheme::WEBSOCKET;
		_secure = false;
		_port = 80;
		s += 5;
	}
	else if (length >= 6 && strncasecmp(s, "wss://", 6) == 0)
	{
		_scheme = Scheme::WEBSOCKET;
		_secure = true;
		_port = 443;
		s += 6;
	}
	else if (length >= 6 && strncasecmp(s, "tcp://", 6) == 0)
	{
		_scheme = Scheme::TCP;
		_port = 0;
		s += 6;
	}
	else if (length >= 6 && strncasecmp(s, "udp://", 6) == 0)
	{
		_scheme = Scheme::UDP;
		_port = 0;
		s += 6;
	}
	else if (length >= 7 && strncasecmp(s, "http://", 7) == 0)
	{
		_scheme = Scheme::HTTP;
		_secure = false;
		_port = 80;
		s += 7;
	}
	else if (length >= 8 && strncasecmp(s, "https://", 8) == 0)
	{
		_scheme = Scheme::HTTP;
		_secure = true;
		_port = 443;
		s += 8;
	}
	else if (length >= 3 && strstr(s, "://") != 0)
	{
		throw std::runtime_error("Wrong scheme");
	}
	else
	{
		_scheme = Scheme::UNDEFINED;
	}

	// Читаем хост
	//host:
	if (*s == '[') // IPv6
	{
		while (*s != ']')
		{
			if (isxdigit(*s) == 0 && *s != ':')
			{
				throw std::runtime_error("Wrong hostname");
			}
			_host.push_back(static_cast<char>(tolower(*s)));
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
		s++;
	}
	else
	{
		while (*s != 0 && *s != ':' && *s != '/' && *s != '?' && *s != '#' && isspace(*s) == 0)
		{
			if (isalnum(*s) == 0 && *s != '.' && *s != '-')
			{
				throw std::runtime_error("Wrong hostname");
			}
			_host.push_back(static_cast<char>(tolower(*s)));
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
	}

	// Читаем порт
	//port:
	if (*s == ':')
	{
		if (++s > end)
		{
			throw std::runtime_error("Out of data");
		}
		uint64_t port = 0;
		while (*s != 0 && *s != '/' && *s != '?' && *s != '#' && isspace(*s) == 0)
		{
			if (isdigit(*s) == 0)
			{
				throw std::runtime_error("Wrong port");
			}
			port = port * 10 + *s - '0';
			if (port < 1 || port > 65535)
			{
				throw std::runtime_error("Wrong port");
			}
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
		_port = static_cast<uint16_t>(port);
	}

	// Читаем путь
	path:
	if (*s == '/')
	{
		while (*s != 0 && *s != '?' && *s != '#' && isspace(*s) == 0)
		{
			_path.push_back(*s);
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
	}

	// Читаем запрос
	//query:
	if (*s == '?')
	{
		if (++s > end)
		{
			throw std::runtime_error("Out of data");
		}
		_hasQuery = true;
		while (*s != 0 && *s != '#' && isspace(*s) == 0)
		{
			_query.push_back(*s);
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
	}

	// Читаем строку GET
	//fragment:
	if (*s == '#')
	{
		if (++s > end)
		{
			throw std::runtime_error("Out of data");
		}
		_hasFragment = true;
		while (*s != 0 && isspace(*s) == 0)
		{
			_fragment.push_back(*s);
			if (++s > end)
			{
				throw std::runtime_error("Out of data");
			}
		}
	}
}

const std::string& URI::str() const
{
	std::stringstream ss;

	if (_scheme == Scheme::HTTP && !_secure) ss << "http://";
	if (_scheme == Scheme::HTTP && _secure) ss << "https://";
	if (_scheme == Scheme::WEBSOCKET && !_secure) ss << "ws://";
	if (_scheme == Scheme::WEBSOCKET && _secure) ss << "wss://";
	if (_scheme == Scheme::TCP) ss << "tcp://";
	if (_scheme == Scheme::UDP) ss << "udp://";
	if (_scheme == Scheme::UNDEFINED && !_host.empty()) ss << "//";

	if (_host.find(':') == std::string::npos)
	{
		ss << _host;
	}
	else
	{
		ss << '[' << _host << ']';
	}

	if (_port != 0)
	{
		if (_scheme == Scheme::HTTP || _scheme == Scheme::WEBSOCKET)
		{
			if ((!_secure && _port != 80) || (_secure && _port != 443))
			{
				ss << ':' << _port;
			}
		}
		else
		{
			ss << ':' << _port;
		}
	}

	ss << _path;

	if (_hasQuery)
	{
		ss << '?' << _query;
	}

	if (_hasFragment)
	{
		ss << '#' << _fragment;
	}

	_thisAsString = ss.str();

	return _thisAsString;
}

std::string URI::urldecode(const std::string& input_)
{
	std::string input(input_);
	std::replace(input.begin(), input.end(), '+', ' ');
	return PercentEncoding::decode(input);
}

std::string URI::urlencode(const std::string& input)
{
	return PercentEncoding::encode(input);
}