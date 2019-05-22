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
// File created on: 2017.03.09

// ConnectionManager.hpp


#pragma once

#include <mutex>
#include <set>
#include <string>
#include <mutex>
#include <map>
#include "Connection.hpp"

class ConnectionManager final
{
public:
	ConnectionManager(const ConnectionManager&) = delete;
	ConnectionManager& operator=(const ConnectionManager&) = delete;
	ConnectionManager(ConnectionManager&& tmp) noexcept = delete;
	ConnectionManager& operator=(ConnectionManager&& tmp) noexcept = delete;

private:
	ConnectionManager();
	~ConnectionManager();

	static ConnectionManager &getInstance()
	{
		static ConnectionManager instance;
		return instance;
	}

	static const int poolSize = 1u<<17;

	Log _log;

	std::recursive_mutex _mutex;

	/// Мютекс для эксклюзивного ожидания событий
	std::mutex _epool_mutex;

	/// Реестр подключений
	std::map<const Connection *, const std::shared_ptr<Connection>> _allConnections;

	/// Захваченные подключения
	std::set<std::shared_ptr<Connection>> _capturedConnections;

	/// Готовые подключения (имеющие необработанные события)
	std::set<std::shared_ptr<Connection>> _readyConnections;

	int _epfd;
	epoll_event _epev[poolSize];

	/// Ожидать события на соединениях
	void wait();

	/// Захватить соединение
	std::shared_ptr<Connection> capture();

	/// Освободить соединение
	void release(const std::shared_ptr<Connection>& conn);

public:
	/// Добавить соединение для наблюдения
	static void watch(const std::shared_ptr<Connection>& connection);

	/// Зарегистрировать соединение
	static void add(const std::shared_ptr<Connection>& connection);

	/// Удалить регистрацию соединения
	static bool remove(const std::shared_ptr<Connection>& connection);

	/// Зарегистрировать таймаут
	static void timeout(const std::shared_ptr<Connection>& connection);

	/// Проверить и вернуть отложенные события
	static uint32_t rotateEvents(const std::shared_ptr<Connection>& connection);

	/// Обработка событий
	static void dispatch();
};
