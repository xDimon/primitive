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

// ConnectionBase.hpp


#pragma once


#include <cstdint>
#include <string>
#include "ConnectionEvent.hpp"
#include <sys/epoll.h>

struct sockaddr_in;

class ConnectionBase
{
private:
	bool _captured;
	uint32_t _events;
	uint32_t _postponedEvents;

protected:
	int _sock;
	bool _ready;
	std::string _name;

public:
	ConnectionBase(const ConnectionBase&) = delete;
	void operator= (ConnectionBase const&) = delete;

	ConnectionBase();
	virtual ~ConnectionBase();

	virtual const std::string& name() = 0;

	inline int fd() const
	{
		return _sock;
	}

	inline bool isReady() const
	{
		return _ready;
	}

	inline void setReleased()
	{
		_captured = false;
		_events = 0;
	}
	inline void setCaptured()
	{
		_captured = true;
	}
	inline bool isCaptured() const
	{
		return _captured;
	}

	uint32_t rotateEvents()
	{
		_events = _postponedEvents;
		_postponedEvents = 0;
		return _events;
	}

	void appendEvents(uint32_t events)
	{
		if (_captured)
		{
			_postponedEvents |= events;
		}
		else
		{
			_events |= events;
		}
	}

	inline bool isReadyForRead() const
	{
		return static_cast<bool>(_events & static_cast<uint32_t>(ConnectionEvent::READ));
	}

	inline bool isReadyForWrite() const
	{
		return static_cast<bool>(_events & static_cast<uint32_t>(ConnectionEvent::WRITE));
	}

	inline bool isHalfHup() const
	{
		return static_cast<bool>(_events & static_cast<uint32_t>(ConnectionEvent::HALFHUP));
	}

	inline bool isHup() const
	{
		return static_cast<bool>(_events & static_cast<uint32_t>(ConnectionEvent::HUP));
	}

	inline bool wasFailure() const
	{
		return static_cast<bool>(_events & static_cast<uint32_t>(ConnectionEvent::ERROR));
	}

	virtual void watch(epoll_event &ev) = 0;

	virtual bool processing() = 0;
};
