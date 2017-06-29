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

// SessionManager.hpp


#pragma once

#include <mutex>
#include <map>
#include "Session.hpp"

class SessionManager
{
private:
	SessionManager() {};
	virtual ~SessionManager() {};

	SessionManager(SessionManager const&) = delete;
	void operator= (SessionManager const&) = delete;

	static SessionManager &getInstance()
	{
		static SessionManager instance;
		return instance;
	}

	/// Сопоставление sid => uid
	std::map<Session::SID, Session::HID> _sid2hid;
	std::mutex _mutexSid2hid;

	/// Пул сессий
	std::map<Session::HID, std::shared_ptr<Session>> _sessions;
	std::recursive_mutex _mutexSessions;

public:
	/// Получить HID по SID
	static Session::HID hidBySid(const Session::SID& sid);

	/// Получить сессию по HID
	static std::shared_ptr<Session> getSession(Session::HID hid);

	/// Закрыть сессию
	static void closeSession(Session::HID hid);

	static std::shared_ptr<Session> putSession(const std::shared_ptr<Session>& session);
};
