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
// File created on: 2017.04.06

// Session.cpp


#include <random>
#include "Session.hpp"
#include "SessionManager.hpp"
#include "../utils/Daemon.hpp"

const char Session::_hidSeed[16] = {};

Session::Session(
	const std::shared_ptr<Service>& service,
	Session::HID uid
)
: _service(service)
, hid(uid)
, _ready(false)
{
	if (!_service)
	{
		throw std::runtime_error("Bad service");
	}
}

void Session::setSid(Session::SID sid)
{
	_sid = std::move(sid);
}

void Session::touch()
{
	if (!_timeoutForClose)
	{
		_timeoutForClose = std::make_shared<Timeout>(
			[wp = std::weak_ptr<std::remove_reference<decltype(*this)>::type>(ptr())](){
				auto session = wp.lock();
				if (session)
				{
					session->close(Daemon::shutingdown()?"shuting down":"timeout");
				}
			}
		);
	}

	_timeoutForClose->restart(timeoutDuration());
}

void Session::changed()
{
	if (!_timeoutForSave)
	{
		_timeoutForSave = std::make_shared<Timeout>(
			[wp = std::weak_ptr<std::remove_reference<decltype(*this)>::type>(ptr())](){
				auto session = wp.lock();
				if (session)
				{
					session->save();
				}
			}
		);
	}

	_timeoutForSave->startOnce(saveDuration());
}

bool Session::load()
{
	return false;
}

bool Session::save()
{
	return false;
}

void Session::close(const std::string& reason)
{
	SessionManager::closeSession(hid);
}
