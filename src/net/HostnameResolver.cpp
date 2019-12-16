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

std::tuple<const char*, std::vector<sockaddr_storage>> HostnameResolver::resolve(std::string host, uint16_t port)
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

	int status;
	addrinfo hints{};
	addrinfo *servinfo;  // указатель на результаты вызова

	memset(&hints, 0, sizeof hints); // убедимся, что структура пуста
	hints.ai_family = AF_UNSPEC;     // неважно, IPv4 или IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &servinfo);

	auto statusMsg = status ? gai_strerror(status) : nullptr;

	std::vector<sockaddr_storage> addrs;

	if (status == 0)
	{
		for (auto info = servinfo; info;  info = info->ai_next)
		{
			sockaddr_storage ss;
			memset(&ss, 0, sizeof(ss));
			if (info->ai_family == AF_INET)
			{
				memcpy(&ss, info->ai_addr, sizeof(sockaddr_in));
				addrs.emplace_back(ss);
			}
			else if (info->ai_family == AF_INET6)
			{
				memcpy(&ss, info->ai_addr, sizeof(sockaddr_in6));
				addrs.emplace_back(ss);
			}
		}
	}

	freeaddrinfo(servinfo);

	{
		std::lock_guard<std::mutex> lockGuard(getInstance()._mutex);
		auto i = getInstance()._cache.find(host);
		if (i != getInstance()._cache.end())
		{
			getInstance()._cache.erase(i);
		}
		getInstance()._cache.emplace(host, std::make_tuple(statusMsg, addrs, time(nullptr) + 3600));
	}

//	Log("HostnameResolver").info("Resolve for '%s' - renew in cache", host.c_str());
	return std::make_tuple(statusMsg, std::move(addrs));
}
