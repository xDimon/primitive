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

#include "../thread/ThreadPool.hpp"
#include "../utils/Daemon.hpp"
#include "../thread/RollbackStackAndRestoreContext.hpp"

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
		getInstance()._log.warn("%s already registered in manager", connection->name().c_str());
		return;
	}

	getInstance()._allConnections.emplace(connection.get(), connection);

	getInstance()._log.debug("%s registered in manager", connection->name().c_str());

	epoll_event ev{};

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_ADD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.warn("Fail add %s for watching (error: '%s')", connection->name().c_str(), strerror(errno));
	}
	else
	{
		getInstance()._log.trace("Add %s for watching", connection->name().c_str());
	}
}

/// Удалить регистрацию соединения
bool ConnectionManager::remove(const std::shared_ptr<Connection>& connection)
{
	if (!connection)
	{
		return false;
	}

	// Удаляем из очереди событий
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_DEL, connection->fd(), nullptr) == -1)
	{
		getInstance()._log.warn("Fail remove %s from watching (error: '%s')", connection->name().c_str(), strerror(errno));
	}
	else
	{
		getInstance()._log.trace("Remove %s from watching", connection->name().c_str());
	}

	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);
	getInstance()._allConnections.erase(connection.get());
	getInstance()._readyConnections.erase(connection);
	getInstance()._capturedConnections.erase(connection);

	getInstance()._log.debug("%s unregistered from manager", connection->name().c_str());

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
//		_log.trace("Skip watch for %s because already captured", connection->name().c_str());
		return;
	}

	epoll_event ev{};

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_MOD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.warn("Fail modify watching on %s (error: '%s')", connection->name().c_str(), strerror(errno));
	}
	else
	{
		getInstance()._log.trace("Modify watching on %s", connection->name().c_str());
	}
}

/// Ожидать события на соединениях
void ConnectionManager::wait()
{
	// Если набор готовых соединений не пустой, то выходим
	if (!_readyConnections.empty())
	{
		return;
	}

	_mutex.unlock();

	_epool_mutex.lock();

	int n = 0;

	while ([&](){std::lock_guard<std::recursive_mutex> lockGuard(_mutex); return _readyConnections.empty();}())
	{
		if ([&](){std::lock_guard<std::recursive_mutex> lockGuard(_mutex); return _allConnections.empty();}())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			break;
		}
		n = epoll_wait(_epfd, _epev, poolSize, 50);
		if (n < 0)
		{
			if (errno != EINTR)
			{
				_log.warn("Error waiting events of connections: fail epoll_wait (%s)", strerror(errno));
				break;
			}
			continue;
		}
		if (n > 0)
		{
			_log.trace("Catch events on %d connection(s)", n);
			break;
		}
		if (Daemon::shutingdown())
		{
			// Закрываем оставшееся
			_epool_mutex.unlock();

			_mutex.lock();

			if (_allConnections.empty())
			{
				_log.debug("Interrupt waiting");
				return;
			}

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

	_mutex.lock();

	// Перебираем полученые события
	for (int i = 0; i < n; i++)
	{
		// Игнорируем незарегистрированные соединения
		auto it = _allConnections.find(static_cast<const Connection *>(_epev[i].data.ptr));
		if (it == _allConnections.end())
		{
			_log.trace("Skip catching of unregistered Connection %p", _epev[i].data.ptr);
			continue;
		}
		auto connection = it->second;

		uint32_t fdEvent = _epev[i].events;
		uint32_t events = 0;

		if (fdEvent & (EPOLLIN | EPOLLRDNORM))
		{
			events |= static_cast<uint32_t>(ConnectionEvent::READ);
		}
		if (fdEvent & (EPOLLOUT | EPOLLWRNORM))
		{
			events |= static_cast<uint32_t>(ConnectionEvent::WRITE);
		}
		if (fdEvent & EPOLLHUP)
		{
			events |= static_cast<uint32_t>(ConnectionEvent::HUP);
		}
		if (fdEvent & EPOLLRDHUP)
		{
			events |= static_cast<uint32_t>(ConnectionEvent::HALFHUP);
		}
		if (fdEvent & EPOLLERR)
		{
			events |= static_cast<uint32_t>(ConnectionEvent::ERROR);
		}

		_log.trace("Catch events `%s` (%04x) on %s", ConnectionEvent::code(events).c_str(), fdEvent, connection->name().c_str());

		connection->appendEvents(events);

		// Если не в списке захваченых...
		if (_capturedConnections.find(connection) == _capturedConnections.end())
		{
			_log.trace("Insert %s into ready connection list and will be processed now (by events)", connection->name().c_str());

			// ...добавляем в список готовых
			_readyConnections.insert(connection);
		}
	}

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
		instance._log.trace("Skip timeout adding for noregistered Connection %p", connection.get());
		return;
	}

	connection->appendEvents(static_cast<uint32_t>(ConnectionEvent::TIMEOUT));

	instance._log.trace("Catch event `T` on %s", connection->name().c_str());

	// Если не в списке захваченых...
	if (instance._capturedConnections.find(connection) == instance._capturedConnections.end())
	{
		instance._log.trace("Insert %s into ready connection list and will be processed now (by timeout)", connection->name().c_str());

		// ...добавляем в список готовых
		instance._readyConnections.insert(connection);
	}
}

/// Захватить соединение
std::shared_ptr<Connection> ConnectionManager::capture()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	// Если нет готовых...
	while (_readyConnections.empty())
	{
		// то выходим при остановке сервера
		if (Daemon::shutingdown() && _allConnections.empty())
		{
			return nullptr;
		}

		_log.trace("Not found ready connection");

		// а в штатном режиме ожидаем появления готового соединения
		wait();
	}

	_log.trace("Found ready connection");

	if (_readyConnections.empty())
	{
		return nullptr;
	}

	// Берем соединение из набора готовых к обработке
	auto it = _readyConnections.begin();

	auto connection = *it;

	// Перемещаем соединение из набора готовых к обработке в набор захваченных
	_readyConnections.erase(it);
	_capturedConnections.insert(connection);

	_log.trace("Capture %s", connection->name().c_str());

	connection->setCaptured();

	return std::move(connection);
}

/// Освободить соединение
void ConnectionManager::release(const std::shared_ptr<Connection>& connection)
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);

	_log.trace("Release %s", connection->name().c_str());

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
	for (;;)
	{
		std::shared_ptr<Connection> connection = getInstance().capture();
		if (!connection)
		{
			break;
		}

		getInstance()._log.debug("Enqueue %s for '%s' events processing", connection->name().c_str(), ConnectionEvent::code(connection->events()).c_str());

		auto task = std::make_shared<Task::Func>(
			[wp = std::weak_ptr<Connection>(connection)](){
				auto connection = wp.lock();
				if (!connection)
				{
					getInstance()._log.trace("Connection death");
					return;
				}

				getInstance()._log.trace("Begin processing on %s", connection->name().c_str());

				bool status;
				try
				{
					status = connection->processing();
				}
				catch (const RollbackStackAndRestoreContext& exception)
				{
					getInstance().release(connection);
					throw;
				}
				catch (const std::exception& exception)
				{
					status = false;
					getInstance()._log.warn("Uncatched exception at processing on %s: %s", connection->name().c_str(), exception.what());
				}

				getInstance().release(connection);

				getInstance()._log.trace("End processing on %s: %s", connection->name().c_str(), status ? "success" : "fail");
			}
		);

		ThreadPool::enqueue(task);
	}
}
