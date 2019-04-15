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
// File created on: 2017.04.06

// SessionManager.cpp


#include "SessionManager.hpp"

bool SessionManager::regSid(const std::shared_ptr<Session>& session, const Session::SID& sid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessionsBySid);
	auto i = getInstance()._sessionsBySid.find(sid);
	if (i != getInstance()._sessionsBySid.end())
	{
		return false;
	}
	if (!session->sid().empty())
	{
		getInstance()._sessionsBySid.erase(session->sid());
	}
	if (sid.empty())
	{
		return true;
	}
	session->setSid(sid);
	getInstance()._sessionsBySid.emplace(session->sid(), session);
	return true;
}

std::shared_ptr<Session> SessionManager::sessionBySid(const Session::SID& sid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessionsBySid);
	auto i = getInstance()._sessionsBySid.find(sid);
	if (i == getInstance()._sessionsBySid.end())
	{
		return {};
	}
	auto session = i->second.lock();
	if (session->sid() != sid)
	{
		getInstance()._sessionsBySid.erase(i);
		return {};
	}
	return session;
}

std::shared_ptr<Session> SessionManager::putSession(const std::shared_ptr<Session>& session, Session::HID hid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessionsByHid);

	auto i = getInstance()._sessionsByHid.find(hid);

	if (i != getInstance()._sessionsByHid.end())
	{
		if (auto oldSession = i->second.lock())
		{
			regSid(oldSession);
			oldSession->touch(true);
			return oldSession;
		}
		getInstance()._sessionsByHid.erase(i);
	}

	if (!session->isReady())
	{
		return {};
	}

	getInstance()._sessionsByHid.emplace(hid, session);
	getInstance()._sessions.emplace(session);

	regSid(session);

	session->touch(true);


	return session;
}

std::shared_ptr<Session> SessionManager::sessionByHid(Session::HID hid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessionsByHid);
	auto i = getInstance()._sessionsByHid.find(hid);

	if (i != getInstance()._sessionsByHid.end())
	{
		if (auto session = i->second.lock())
		{
			return session;
		}
		getInstance()._sessionsByHid.erase(i);
	}

	return nullptr;
}

void SessionManager::closeSession(const std::shared_ptr<Session>& session)
{
	if (!session)
	{
		return;
	}

	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessions);

	auto i = getInstance()._sessions.find(session);
	if (i == getInstance()._sessions.end())
	{
		return;
	}

	if (!session->sid().empty())
	{
		std::lock_guard<std::mutex> lockGuard2(getInstance()._mutexSessionsBySid);
		getInstance()._sessionsBySid.erase(session->sid());
	}

	getInstance()._sessions.erase(i);
}

void SessionManager::forEach(const std::function<void(const std::shared_ptr<Session>&)>& handler)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessions);

	for (auto& session : getInstance()._sessions)
	{
		handler(session);
	}
}
