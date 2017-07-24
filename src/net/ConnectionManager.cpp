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

// ConnectionManager.cpp


#include <cstring>
#include "ConnectionManager.hpp"

#include <unistd.h>
#include "../thread/ThreadPool.hpp"
#include "TcpConnection.hpp"
#include "../utils/ShutdownManager.hpp"

ConnectionManager::ConnectionManager()
: _log("ConnectionManager")
{
	_epfd = epoll_create(poolSize);
	memset(_epev, 0, sizeof(_epev));
}

ConnectionManager::~ConnectionManager()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	std::vector<std::shared_ptr<Connection>> connections;
	for (auto& i : _allConnections)
	{
		connections.emplace_back(i.second);
	}
	for (auto& connection : connections)
	{
		remove(connection);
	}
	close(_epfd);
	_epfd = -1;
}

/// Зарегистрировать соединение
void ConnectionManager::add(const std::shared_ptr<Connection>& connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	if (getInstance()._allConnections.find(connection.get()) != getInstance()._allConnections.end())
	{
		getInstance()._log.debug("Fail of add %s in ConnectionManager::add()", connection->name().c_str());
		return;
	}

	getInstance()._log.debug("Add %s in ConnectionManager::add()", connection->name().c_str());

	getInstance()._allConnections.emplace(connection.get(), connection);

	epoll_event ev;

	connection->watch(ev);

	getInstance()._log.trace("Call `epoll_ctl(ADD)` for %s in ConnectionManager::add(): %p", connection->name().c_str(), ev.data.ptr);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_ADD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(ADD)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}
}

/// Удалить регистрацию соединения
bool ConnectionManager::remove(const std::shared_ptr<Connection>& connection)
{
	if (!connection)
	{
		return false;
	}

	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	getInstance()._allConnections.erase(connection.get());

	getInstance()._log.trace("Call `epoll_ctl(DEL)` for %s in ConnectionManager::add()", connection->name().c_str());

	// Удаляем из очереди событий
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_DEL, connection->fd(), nullptr) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(DEL)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}

	getInstance()._readyConnections.erase(connection);
	getInstance()._capturedConnections.erase(connection);

	return true;
}

uint32_t ConnectionManager::rotateEvents(const std::shared_ptr<Connection>& connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);
	uint32_t events = connection->rotateEvents();
	return events;
}

void ConnectionManager::watch(const std::shared_ptr<Connection>& connection)
{
	// Для известных соенинений проверяем состояние захваченности
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	// Те, что в обработке, не трогаем
	if (connection->isCaptured())
	{
//		_log.trace("Skip watch for %s in ConnectionManager::watch() because already captured", connection->name().c_str());
		return;
	}

//	_log.trace("Watch for %s in ConnectionManager::watch()", connection->name().c_str());

	epoll_event ev;

	connection->watch(ev);

	getInstance()._log.trace("Call `epoll_ctl(MOD)` for %s in ConnectionManager::watch(): %p", connection->name().c_str(), ev.data.ptr);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_MOD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(MOD)` for %s (error: '%s') in ConnectionManager::watch()", connection->name().c_str(), strerror(errno));
	}
}

/// Ожидать события на соединениях
void ConnectionManager::wait()
{
	// Если набор готовых соединений не пустой, то выходим
	if (!_readyConnections.empty())
	{
//		_log.trace("No wait in ConnectionManager::wait() because have ready connection");
		return;
	}

	_mutex.unlock();

	_epool_mutex.lock();

//	_log.trace("Begin waiting in ConnectionManager::wait()");

	int n;

	for (;;)
	{
		n = epoll_wait(_epfd, _epev, poolSize, 50);
		if (n < 0)
		{
			if (errno != EINTR)
			{
				_log.warn("epoll_wait error (%s) in ConnectionManager::wait()", strerror(errno));
			}
			continue;
		}
		if (n > 0)
		{
			_log.trace("Catch event(s) on %d connection(s) in ConnectionManager::wait()", n);
			break;
		}
		if (ShutdownManager::shutingdown())
		{
			if (_allConnections.empty())
			{
				_log.debug("Interrupt waiting");

				_epool_mutex.unlock();

				_mutex.lock();
				return;
			}
			// Если можно закрыть все оставшееся - делаем это
			else
			{
				_epool_mutex.unlock();

				_mutex.lock();

				std::vector<std::shared_ptr<Connection>> connections;
				for (auto& i : _allConnections)
				{
					connections.emplace_back(i.second);
				}
				for (auto& connection : connections)
				{
					remove(connection);
				}

				_mutex.unlock();

				_epool_mutex.lock();
			}
		}
	}

	_mutex.lock();

	// Перебираем полученые события
	for (int i = 0; i < n; i++)
	{
		// Игнорируем незарегистрированные соединения
		auto it = _allConnections.find(static_cast<const Connection *>(_epev[i].data.ptr));
		if (it == _allConnections.end())
		{
			_log.trace("Skip Connection#%p in ConnectionManager::wait() because unregistered", _epev[i].data.ptr);
			continue;
		}

		auto connection = it->second;

		_log.trace("Catch event(s) `%d` on %s in ConnectionManager::wait(): %p", _epev[i].events, connection->name().c_str(), _epev[i].data.ptr);

		uint32_t fdEvent = _epev[i].events;

		uint32_t events = 0;
		if (fdEvent & (EPOLLIN | EPOLLRDNORM))
		{
			_log.trace("Catch event EPOLLIN on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::READ);
		}
		if (fdEvent & (EPOLLOUT | EPOLLWRNORM))
		{
			_log.trace("Catch event EPOLLOUT on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::WRITE);
		}
		if (fdEvent & EPOLLHUP)
		{
			_log.trace("Catch event EPOLLEHUP on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::HUP);
		}
		if (fdEvent & EPOLLRDHUP)
		{
			_log.trace("Catch event EPOLLERDHUP on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::HALFHUP);
		}
		if (fdEvent & EPOLLERR)
		{
			_log.trace("Catch event EPOLLERR on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::ERROR);
		}

		connection->appendEvents(events);

		// Если не в списке захваченых...
		if (_capturedConnections.find(connection) == _capturedConnections.end())
		{
			_log.trace(
				"Insert %s into ready connection list and will be processed now in ConnectionManager::wait()", connection->name().c_str());

			// ...добавляем в список готовых
			_readyConnections.insert(connection);
		}
	}

//	_log.trace("End waiting in ConnectionManager::wait()");

	_epool_mutex.unlock();
}

/// Зарегистрировать таймаут
void ConnectionManager::timeout(const std::shared_ptr<Connection>& connection)
{
	auto& instance = getInstance();

	std::lock_guard<std::recursive_mutex> lockGuard(instance._mutex);

	// Игнорируем незарегистрированные соединения
	auto it = instance._allConnections.find(connection.get());
	if (it == instance._allConnections.end())
	{
		instance._log.trace("Skip Connection#%p in ConnectionManager::timeout() because unregistered", connection.get());
		return;
	}

	connection->appendEvents(static_cast<uint32_t>(ConnectionEvent::TIMEOUT));

	// Если не в списке захваченых...
	if (instance._capturedConnections.find(connection) == instance._capturedConnections.end())
	{
		instance._log.trace("Insert %s into ready connection list and will be processed now in ConnectionManager::timeout()", connection->name().c_str());

		// ...добавляем в список готовых
		instance._readyConnections.insert(connection);
	}
}

/// Захватить соединение
std::shared_ptr<Connection> ConnectionManager::capture()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	_log.trace("Try capture connection in ConnectionManager::capture()");

	// Если нет готовых...
	while (_readyConnections.empty())
	{
		// то выходим при остановке сервера
		if (ShutdownManager::shutingdown() && _allConnections.empty())
		{
			return nullptr;
		}

		_log.trace("Not found ready connection in ConnectionManager::capture()");

		// а в штатном режиме ожидаем появления готового соединения
		wait();
	}

	_log.trace("Found ready connection in ConnectionManager::capture()");

	if (_readyConnections.empty())
	{
		return nullptr;
	}

	// Берем соединение из набора готовых к обработке
	auto it = _readyConnections.begin();

	auto connection = *it;

//	_log.trace("Move %s from ready into captured connection list in ConnectionManager::captured()", connection->name().c_str());

	// Перемещаем соединение из набора готовых к обработке в набор захваченных
	_readyConnections.erase(it);
	_capturedConnections.insert(connection);

	_log.trace("Capture %s in ConnectionManager::capture()", connection->name().c_str());

	connection->setCaptured();

	return std::move(connection);
}

/// Освободить соединение
void ConnectionManager::release(const std::shared_ptr<Connection>& connection)
{
//	_log.trace("Enter into ConnectionManager::release() for %s", connection->name().c_str());

	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	_log.trace("Remove %s from captured connection list in ConnectionManager::captured()", connection->name().c_str());

	_capturedConnections.erase(connection);

	connection->setReleased();

	if (!connection->isClosed())
	{
		watch(connection);
	}
}

/// Обработка событий
void ConnectionManager::dispatch()
{
//	getInstance()._log.debug("Start dispatching in ConnectionManager::dispatch()");
//
	for (;;)
	{
		std::shared_ptr<Connection> connection = getInstance().capture();

		if (!connection)
		{
			break;
		}

		getInstance()._log.debug("Enqueue %s for procession", connection->name().c_str());

		auto task = std::make_shared<Task::Func>(
			[connection](){
				getInstance()._log.trace("Begin processing on %s", connection->name().c_str());

				bool status = connection->processing();

				getInstance().release(connection);

				getInstance()._log.trace("End processing on %s: %s", connection->name().c_str(), status ? "success" : "fail");
			}
		);

		ThreadPool::enqueue(task);
	}
}
