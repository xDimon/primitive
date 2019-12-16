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

// URI.hpp


#pragma once

#include <string>

class URI final
{
public:
	enum class Scheme {
		UNDEFINED,
		TCP,
		UDP,
		HTTP,
		WEBSOCKET
	};

private:
	Scheme _scheme;
	bool _secure;
	std::string _host;
	uint16_t _port;
	std::string _path;
	bool _hasQuery;
	std::string _query;
	bool _hasFragment;
	std::string _fragment;
	mutable std::string _thisAsString;

public:
	URI();
	URI(std::string_view uri);
	virtual ~URI() = default;

	void parse(const char* string, size_t length);
	void parse(const std::string& string)
	{
		parse(string.c_str(), string.size());
	};

	const std::string& str() const;

	bool isSecure() const
	{
		return _secure;
	}

	Scheme scheme() const
	{
		return _scheme;
	}
	void setScheme(Scheme scheme, bool secure = false)
	{
		_scheme = scheme;
		_secure = secure;
		if (_port == 0)
		{
			if (_scheme == Scheme::HTTP || _scheme == Scheme::WEBSOCKET)
			{
				_port = _secure ? 443 : 80;
			}
		}
	}

	const std::string& host() const
	{
		return _host;
	}
	void setHost(const std::string& host)
	{
		_host = host;
	}

	uint16_t port() const
	{
		return _port;
	}
	void setPort(uint16_t port)
	{
		_port = port;
	}

	const std::string& path() const
	{
		return _path;
	}
	void setPath(const std::string& path)
	{
		_path = path;
	}

	bool hasQuery() const
	{
		return _hasQuery;
	}
	const std::string& query() const
	{
		return _query;
	}
	void setQuery(const std::string& query)
	{
		_query = query;
		if (!_query.empty())
		{
			_hasQuery = true;
		}
	}

	bool hasFragment() const
	{
		return _hasFragment;
	}
	const std::string& fragment() const
	{
		return _fragment;
	}
	void setFragment(const std::string& fragment)
	{
		_fragment = fragment;
		if (!_fragment.empty())
		{
			_hasFragment = true;
		}
	}

	static std::string urldecode(const std::string& input);

	static std::string urlencode(const std::string& input);
};