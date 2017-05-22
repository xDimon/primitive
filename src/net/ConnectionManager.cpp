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

	_accepted = 0;
	_established = 0;
	_establishedMax = 0;

	_size = 0;
}

ConnectionManager::~ConnectionManager()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);
	close(_epfd);
	_epfd = -1;
	for (auto i = _allConnections.begin(); i != _allConnections.end();)
	{
		auto ci = i++;
		auto connection = *ci;
		_allConnections.erase(ci);
		remove(connection);
	}
}

/// Зарегистрировать соединение
void ConnectionManager::add(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	if (getInstance()._allConnections.find(connection) != getInstance()._allConnections.end())
	{
		getInstance()._log.debug("Fail of add %s in ConnectionManager::add()", connection->name().c_str());
		return;
	}

	getInstance()._log.debug("Add %s in ConnectionManager::add()", connection->name().c_str());

	getInstance()._allConnections.insert(connection);

	epoll_event ev;

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_ADD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(ADD)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}
}

/// Удалить регистрацию соединения
bool ConnectionManager::remove(std::shared_ptr<Connection> connection)
{
	if (!connection)
	{
		return false;
	}

	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	if (getInstance()._allConnections.erase(connection))
	{
		if (std::dynamic_pointer_cast<TcpConnection>(connection))
		{
			getInstance()._established--;
		}
	}

	// Удаляем из очереди событий
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_DEL, connection->fd(), nullptr) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(DEL)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}

	getInstance()._readyConnections.erase(connection);
	getInstance()._capturedConnections.erase(connection);

	return true;
}

uint32_t ConnectionManager::rotateEvents(std::shared_ptr<Connection> connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);
	uint32_t events = connection->rotateEvents();
	return events;
}

void ConnectionManager::watch(std::shared_ptr<Connection> connection)
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

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_MOD, connection->fd(), &ev) == -1)
	{
		getInstance()._log.debug("Fail call `epoll_ctl(MOD)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
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
				std::this_thread::sleep_for(std::chrono::seconds(1));
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

		}
	}

	_mutex.lock();

	// Перебираем полученые события
	for (int i = 0; i < n; i++)
	{
		std::shared_ptr<Connection> connection = std::move(*static_cast<std::shared_ptr<Connection> *>(_epev[i].data.ptr));

		if (!connection)
		{
			_log.trace("Skip nullptr in ConnectionManager::wait(): %p", _epev[i].data.ptr);
			continue;
		}

		_log.trace("Catch event(s) `%d` on %s in ConnectionManager::wait()", _epev[i].events, connection->name().c_str());

		// Игнорируем незарегистрированные соединения
		if (_allConnections.find(connection) == _allConnections.end())
		{
			_log.warn("Skip %s in ConnectionManager::wait() because connection unregistered", connection->name().c_str());
			continue;
		}

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

	return connection;
}

/// Освободить соединение
void ConnectionManager::release(std::shared_ptr<Connection> connection)
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
	getInstance()._log.debug("Start dispatching in ConnectionManager::dispatch()");

	for (;;)
	{
		std::shared_ptr<Connection> connection = getInstance().capture();

		if (!connection)
		{
			break;
		}

//		_log.trace("Enqueue %s into ThreadPool for procession ConnectionManager::dispatch()", connection->name().c_str());

		ThreadPool::enqueue([connection](){
			getInstance()._log.trace("Begin processing for %s", connection->name().c_str());

			connection->processing();

			getInstance().release(connection);

			getInstance()._log.trace("End processing for %s", connection->name().c_str());
		});
	}
}
