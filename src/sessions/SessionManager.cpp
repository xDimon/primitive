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
// File created on: 2017.04.06

// SessionManager.cpp


#include "SessionManager.hpp"

Session::HID SessionManager::hidBySid(const Session::SID& sid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSid2hid);
	auto i = getInstance()._sid2hid.find(sid);
	return (i != getInstance()._sid2hid.end()) ? i->second : 0;
}

std::shared_ptr<Session> SessionManager::putSession(const std::shared_ptr<Session>& session)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(session->hid);

	if (i != getInstance()._sessions.end())
	{
		i->second->touch();
		return i->second;
	}

	if (!session->isReady())
	{
		return nullptr;
	}

	getInstance()._sessions.emplace(session->hid, session);

	session->touch();
	return session;
}

std::shared_ptr<Session> SessionManager::getSession(Session::HID uid)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(uid);

	if (i != getInstance()._sessions.end())
	{
		return i->second;
	}

	return nullptr;
}

void SessionManager::closeSession(Session::HID uid)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(uid);

	if (i == getInstance()._sessions.end())
	{
		return;
	}

	auto sid = i->second->sid();
	if (sid != 0)
	{
		std::lock_guard<std::mutex> lockGuard2(getInstance()._mutexSid2hid);
		getInstance()._sid2hid.erase(sid);
	}

	getInstance()._sessions.erase(i);
}
