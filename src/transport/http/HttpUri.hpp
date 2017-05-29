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

// HttpUri.hpp


#pragma once

#include <string>

class HttpUri
{
public:
	enum class Scheme {
		UNDEFINED,
		HTTP,
		HTTPS
	};

private:
	Scheme _scheme;
	std::string _host;
	uint16_t _port;
	std::string _path;
	bool _hasQuery;
	std::string _query;
	bool _hasFragment;
	std::string _fragment;
	mutable std::string _thisAsString;

public:
	HttpUri();
	virtual ~HttpUri();

	void parse(const char* string, size_t length);
	void parse(const std::string string)
	{
		parse(string.c_str(), string.size());
	};

	const std::string& str() const;

	Scheme scheme() const
	{
		return _scheme;
	}
	const std::string& host() const
	{
		return _host;
	}
	uint16_t port() const
	{
		return _port;
	}
	const std::string& path() const
	{
		return _path;
	}
	bool hasQuery() const
	{
		return _hasQuery;
	}
	const std::string& query() const
	{
		return _query;
	}
	bool hasFragment() const
	{
		return _hasFragment;
	}
	const std::string& fragment() const
	{
		return _fragment;
	}

	void assignHost(const std::string& host)
	{
		_host = host;
	}

	static std::string urldecode(const std::string &encoded);

	static std::string urlencode(const std::string& value);
};
