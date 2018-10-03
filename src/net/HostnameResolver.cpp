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
// File created on: 2017.12.11

// HostnameResolver.cpp


#include <netdb.h>
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <cstring>
#include <mutex>
#include "HostnameResolver.hpp"
#include "../log/Log.hpp"

std::tuple<int, std::vector<in_addr>> HostnameResolver::resolve(std::string host)
{
	std::transform(host.begin(), host.end(), host.begin(), ::tolower);

	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		auto i = getInstance()._cache.find(host);
		if (i != getInstance()._cache.end() && std::get<2>(i->second) > time(nullptr))
		{
//			Log("HostnameResolver").info("Resolve for '%s' - get from cache", host.c_str());
			return std::make_tuple(std::get<0>(i->second), std::get<1>(i->second));
		}
	}

	std::vector<char> _buff(1024, 0);

	int herr = 0;
	hostent _hostbuf{};
	hostent* _hostptr;

	// Список адресов по хосту
	while (gethostbyname_r(host.c_str(), &_hostbuf, _buff.data(), _buff.size(), &_hostptr, &herr) == ERANGE)
	{
		try
		{
			_buff.resize(_buff.size() << 1);
		}
		catch (const std::bad_alloc& )
		{
			_buff.resize(_buff.size() + 64);
		}
	}

	std::vector<in_addr> addrs;

	if (_hostptr != nullptr)
	{
		for (auto i = _hostptr->h_addr_list; *i; ++i)
		{
			in_addr addr;
			memcpy(&addr, *i, sizeof(addr));
			addrs.push_back(addr);
		}
	}

	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		auto i = getInstance()._cache.find(host);
		if (i != getInstance()._cache.end())
		{
			getInstance()._cache.erase(i);
		}
		getInstance()._cache.emplace(host, std::make_tuple(herr, addrs, time(nullptr) + 3600));
	}

//	Log("HostnameResolver").info("Resolve for '%s' - renew in cache", host.c_str());
	return std::make_tuple(herr, std::move(addrs));
}
