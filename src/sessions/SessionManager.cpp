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

SessionManager::SessionManager()
{

}

SessionManager::~SessionManager()
{

}

Session::UID SessionManager::getUid(const Session::SID& sid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSid2uid);
	auto i = getInstance()._sid2uid.find(sid);
	return (i != getInstance()._sid2uid.end()) ? i->second : 0;
}

std::shared_ptr<Session> SessionManager::getSession(Session::UID uid, bool load)
{
	{
		std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
		auto i = getInstance()._sessions.find(uid);

		if (i != getInstance()._sessions.end())
		{
			return i->second;
		}

		if (!load)
		{
			return std::move(std::shared_ptr<Session>());
		}
	}

	auto session = std::make_shared<Session>(uid);
	if (!session->isReady())
	{
		return std::shared_ptr<Session>();
	}

	{
		std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
		auto i = getInstance()._sessions.find(uid);

		if (i != getInstance()._sessions.end())
		{
			return i->second;
		}

		getInstance()._sessions.emplace(uid, session);
	}

	return session;
}

void SessionManager::closeSession(Session::UID uid)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(uid);

	if (i == getInstance()._sessions.end())
	{
		return;
	}

	auto& sid = i->second->sid();
	if (sid.size())
	{
		std::lock_guard<std::mutex> lockGuard2(getInstance()._mutexSid2uid);
		getInstance()._sid2uid.erase(sid);
	}

	getInstance()._sessions.erase(i);
}
