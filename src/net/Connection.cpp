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
// File created on: 2017.03.09

// Connection.cpp


#include <unistd.h>
#include "Connection.hpp"

#include <unistd.h>
#include <sstream>

Connection::Connection(const std::shared_ptr<Transport>& transport)
: _log("Connection")
, _transport(transport)
, _sock(-1)
, _timeout(false)
, _closed(true)
, _error(false)
, _realExpireTime(std::chrono::steady_clock::now())
, _nextExpireTime(std::chrono::time_point<std::chrono::steady_clock>::max())
{
	_captured = false;
	_events = 0;
	_postponedEvents = 0;

	std::ostringstream ss;
	ss << "[__connection#" << this << "][" << _sock << "]";
	_name = std::move(ss.str());

	_ready = false;
}

Connection::~Connection()
{
	if (_sock != -1)
	{
		::close(_sock);
	}
}

void Connection::setTtl(std::chrono::milliseconds ttl)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto now = std::chrono::steady_clock::now();
	_realExpireTime = now + ttl;

	auto prev = _nextExpireTime;
	_nextExpireTime = std::min(_realExpireTime, std::max(now, _nextExpireTime));

	if (prev <= _nextExpireTime)
	{
		return;
	}

	if (!_timeoutWatcher)
	{
		_timeoutWatcher = std::make_shared<TimeoutWatcher>(ptr());
	}

	_timeoutWatcher->restart(_nextExpireTime);
}
