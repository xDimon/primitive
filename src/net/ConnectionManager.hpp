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

// ConnectionManager.hpp


#pragma once

#include <mutex>
#include <set>
#include <string>
#include <mutex>
#include "ConnectionBase.hpp"

class ConnectionManager
{
private:
	ConnectionManager();
	~ConnectionManager();
	ConnectionManager(ConnectionManager const&) = delete;
	void operator= (ConnectionManager const&) = delete;

	static ConnectionManager &getInstance()
	{
		static ConnectionManager instance;
		return instance;
	}

	static const int poolSize = 1<<17;

	std::recursive_mutex _mutex;

	/// Мютекс для эксклюзивного ожидания событий
	std::mutex _epool_mutex;

	/// Реестр подключений
	std::set<ConnectionBase::Ptr> _aConns;

	/// Захваченные подключения
	std::set<ConnectionBase::Ptr> _capturedConnections;

	/// Готовые подключения (имеющие необработанные события)
	std::set<ConnectionBase::Ptr> _readyConnections;

	uint32_t _accepted;
	uint32_t _established;
	uint32_t _establishedMax;

	// Количество одновременно обслуживаемых соединений
	int _size;

	int _epfd;
	epoll_event _epev[poolSize];

	/// Ожидать события на соединениях
	void wait();

	/// Захватить соединение
	ConnectionBase::Ptr capture();

	/// Освободить соединение
	void release(ConnectionBase::Ptr conn);

public:
	/// Добавить соединение для наблюдения
	static void watch(ConnectionBase::Ptr connection);

	/// Зарегистрировать соединение
	static void add(ConnectionBase::Ptr connection);

	/// Удалить регистрацию соединения
	static bool remove(ConnectionBase::Ptr connection);

	/// Проверить и вернуть отложенные события
	static uint32_t rotateEvents(ConnectionBase::Ptr connection);

	/// Обработка событий
	static void dispatch();
};
