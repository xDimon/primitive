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

// HttpUri.cpp


#include <cstring>
#include <stdexcept>
#include <sstream>
#include "HttpUri.hpp"

HttpUri::HttpUri()
: _scheme(Scheme::UNDEFINED)
, _port(0)
, _hasQuery(false)
, _hasFragment(false)
{
}

HttpUri::~HttpUri()
{
}

void HttpUri::parse(const char *string, size_t length)
{
	auto s = string;

	// Читаем схему
	if (!strncmp(s, "//", 2))
	{
		_scheme = Scheme::UNDEFINED;
		s += 2;
	}
	else if (*s == '/')
	{
		_scheme = Scheme::UNDEFINED;
		goto path;
	}
	else if (!strncasecmp(s, "http://", 7))
	{
		_scheme = Scheme::HTTP;
		s += 7;
	}
	else if (!strncasecmp(s, "https://", 8))
	{
		_scheme = Scheme::HTTPS;
		s += 8;
	}
	else
	{
		throw std::runtime_error("Bad URI: wrong scheme");
	}

	// Читаем хост
	//host:
	while (*s && *s != ':' && *s != '/' && *s != '?' && *s != '#' && !isspace(*s))
	{
		if (!isalnum(*s) && *s != '.' && *s != '-')
		{
			throw std::runtime_error("Bad URI: wrong hostname");
		}
		_host.push_back(static_cast<char>(tolower(*s)));
		s++;
	}

	// Читаем порт
	//port:
	if (*s == ':')
	{
		s++;
		_port = 0;
		while (*s && *s != '/' && *s != '?' && *s != '#' && !isspace(*s))
		{
			if (!isdigit(*s))
			{
				throw std::runtime_error("Bad URI: wrong port");
			}
			_host.push_back(*s++);
		}
	}

	// Читаем путь
	path:
	if (*s == '/')
	{
		while (*s && *s != '?' && *s != '#' && !isspace(*s))
		{
			_path.push_back(*s++);
		}
	}

	// Читаем запрос
	//query:
	if (*s == '?')
	{
		s++;
		_hasQuery = true;
		while (*s && *s != '#' && !isspace(*s))
		{
			_query.push_back(*s++);
		}
	}

	// Читаем строку GET
	//fragment:
	if (*s == '#')
	{
		s++;
		_hasFragment = true;
		while (*s && !isspace(*s))
		{
			_fragment.push_back(*s++);
		}
	}
}

const std::string& HttpUri::str() const
{
	std::stringstream ss;

	if (_scheme == Scheme::HTTP) ss << "http://";
	if (_scheme == Scheme::HTTPS) ss << "https://";
	if (_scheme == Scheme::UNDEFINED && !_host.empty()) ss << "//";

	ss << _host;

	if (_port != 0 && ((_scheme == Scheme::HTTP && _port != 80) || (_scheme == Scheme::HTTPS && _port != 443)))
	{
		ss << ":" << _port;
	}

	ss << _path;

	if (_hasQuery)
	{
		ss << "?" << _query;
	}

	if (_hasFragment)
	{
		ss << "#" << _fragment;
	}

	_thisAsString = std::move(ss.str());

	return _thisAsString;
}
