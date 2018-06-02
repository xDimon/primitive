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

// SessionManager.cpp


#include "SessionManager.hpp"

bool SessionManager::regSid(const std::shared_ptr<Session>& session, const Session::SID& sid)
{
	std::lock_guard<std::mutex> lockGuard(getInstance()._mutexSessionsBySid);
	if (!session->sid().empty())
	{
		getInstance()._sessionsBySid.erase(session->sid());
	}
	if (sid.empty())
	{
		return true;
	}
	auto i = getInstance()._sessionsBySid.find(sid);
	if (i != getInstance()._sessionsBySid.end())
	{
		return false;
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
		return nullptr;
	}
	auto session = i->second.lock();
	if (session->sid() != sid)
	{
		getInstance()._sessionsBySid.erase(i);
		return nullptr;
	}
	return std::move(session);
}

std::shared_ptr<Session> SessionManager::putSession(const std::shared_ptr<Session>& session)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(session->hid);

	if (i != getInstance()._sessions.end())
	{
		regSid(session);
		i->second->touch();
		return i->second;
	}

	if (!session->isReady())
	{
		return nullptr;
	}

	getInstance()._sessions.emplace(session->hid, session);

	regSid(session);

	session->touch();
	return session;
}

std::shared_ptr<Session> SessionManager::getSession(Session::HID hid)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);
	auto i = getInstance()._sessions.find(hid);

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
	if (!sid.empty())
	{
		std::lock_guard<std::mutex> lockGuard2(getInstance()._mutexSessionsBySid);
		getInstance()._sessionsBySid.erase(sid);
	}

	getInstance()._sessions.erase(i);
}

void SessionManager::forEach(const std::function<void(const std::shared_ptr<Session>&)>& handler)
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutexSessions);

	for (auto i : getInstance()._sessions)
	{
		handler(i.second);
	}
}
