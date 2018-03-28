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

// SessionManager.hpp


#pragma once

#include <mutex>
#include <map>
#include "Session.hpp"

class SessionManager final
{
public:
	SessionManager(SessionManager const&) = delete;
	void operator= (SessionManager const&) = delete;
	SessionManager(SessionManager&&) noexcept = delete;
	SessionManager& operator=(SessionManager&&) noexcept = delete;

private:
	SessionManager() = default;
	~SessionManager() = default;

	static SessionManager &getInstance()
	{
		static SessionManager instance;
		return instance;
	}

	/// Сопоставление sid => session
	std::map<Session::SID, const std::weak_ptr<Session>> _sessionsBySid;
	std::mutex _mutexSessionsBySid;

	/// Пул сессий
	std::map<Session::HID, const std::shared_ptr<Session>> _sessions;
	std::recursive_mutex _mutexSessions;

public:
	/// Зарегистрировать SID
	static bool regSid(const std::shared_ptr<Session>& session, const Session::SID& sid = "");

	/// Получить HID по SID
	static std::shared_ptr<Session> sessionBySid(const Session::SID& sid);

	/// Получить сессию по HID
	static std::shared_ptr<Session> getSession(Session::HID hid);

	/// Закрыть сессию
	static void closeSession(Session::HID hid);

	static std::shared_ptr<Session> putSession(const std::shared_ptr<Session>& session);

	static void forEach(const std::function<void(const std::shared_ptr<Session>&)>& handler);
};
