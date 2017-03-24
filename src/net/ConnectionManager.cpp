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

#include "TcpConnection.hpp"
#include <unistd.h>

ConnectionManager::ConnectionManager()
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
	for (auto i = _aConns.begin(); i != _aConns.end();)
	{
		auto ci = i++;
		auto connection = *ci;
		_aConns.erase(ci);
		remove(connection);
	}
}

/// Зарегистрировать соединение
void ConnectionManager::add(ConnectionBase::Ptr connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	if (getInstance()._aConns.find(connection) != getInstance()._aConns.end())
	{
		Log().debug("Fail of add {} in ConnectionManager::add()", connection->name());
		return;
	}

	Log().debug("Add {} in ConnectionManager::add()", connection->name());

	getInstance()._aConns.insert(connection);

	epoll_event ev;

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_ADD, connection->fd(), &ev) == -1)
	{
		Log().debug("Fail call `epoll_ctl(ADD)` for {} (error: '{}') in ConnectionManager::add()", connection->name(), strerror(errno));
	}
}

/// Удалить регистрацию соединения
bool ConnectionManager::remove(ConnectionBase::Ptr connection)
{
	if (!connection)
	{
		return false;
	}

	Log().debug("Remove {} in ConnectionManager::remove()", connection->name());

	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	Log().debug("Count of use for {} before ConnectionManager::remove(): {}", connection->name(), connection.use_count());

	if (getInstance()._aConns.erase(connection))
	{
		if (std::dynamic_pointer_cast<TcpConnection>(connection))
		{
			getInstance()._established--;
		}
	}

	// Удаляем из очереди событий
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_DEL, connection->fd(), nullptr) == -1)
	{
		Log().debug("Fail call `epoll_ctl(DEL)` for {} (error: '{}') in ConnectionManager::add()", connection->name(), strerror(errno));
	}

	getInstance()._readyConnections.erase(connection);

	getInstance()._capturedConnections.erase(connection);

	Log().debug("Count of use for {} after ConnectionManager::remove(): {}", connection->name(), connection.use_count());

	return true;
}

uint32_t ConnectionManager::rotateEvents(ConnectionBase::Ptr connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);
	uint32_t events = connection->rotateEvents();
	return events;
}

void ConnectionManager::watch(ConnectionBase::Ptr connection)
{
	// Для известных соенинений проверяем состояние захваченности
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	// Те, что в обработке, не трогаем
	if (connection->isCaptured())
	{
//		Log().trace("Skip watch for {} in ConnectionManager::watch() because already captured", connection->name());
		return;
	}

//	Log().trace("Watch for {} in ConnectionManager::watch()", connection->name());

	epoll_event ev;

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_MOD, connection->fd(), &ev) == -1)
	{
		Log().debug("Fail call `epoll_ctl(MOD)` for {} (error: '{}') in ConnectionManager::add()", connection->name(), strerror(errno));
	}
}

/// Ожидать события на соединениях
void ConnectionManager::wait()
{
	ConnectionBase::Ptr connection;

	// Если набор готовых соединений не пустой, то выходим
	if (!_readyConnections.empty())
	{
//		Log().trace("No wait in ConnectionManager::wait() because have ready connection");
		return;
	}

	_mutex.unlock();

	_epool_mutex.lock();

//	Log().trace("Begin waiting in ConnectionManager::wait()");

	int n;

	for (;;)
	{
		n = epoll_wait(_epfd, _epev, poolSize, 50);
		if (n < 0)
		{
			if (errno != EINTR)
			{
				Log().warn("epoll_wait error ({}) in ConnectionManager::wait()", strerror(errno));
			}
			continue;
		}
		if (n > 0)
		{
			Log().trace("Catch event(s) on {} connection(s) in ConnectionManager::wait()", n);
			break;
		}
	}

	_mutex.lock();

	// Перебираем полученые события
	for (int i = 0; i < n; i++)
	{
		connection = *static_cast<std::shared_ptr<ConnectionBase> *>(_epev[i].data.ptr);

		if (!connection)
		{
			Log().trace("Skip nullptr in ConnectionManager::wait(): {:p}", _epev[i].data.ptr);
			continue;
		}

		Log().trace("Catch event(s) `{}` on {} in ConnectionManager::wait()", _epev[i].events, connection->name());

		// Игнорируем незарегистрированные соединения
		if (_aConns.find(connection) == _aConns.end())
		{
			Log().warn("Skip {} in ConnectionManager::wait() because connection unregistered", connection->name());
			continue;
		}

		uint32_t fdEvent = _epev[i].events;

		uint32_t events = 0;
		if (fdEvent & (EPOLLIN | EPOLLRDNORM))
		{
			Log().trace("Catch event EPOLLIN on {} in ConnectionManager::wait()", connection->name());
			events |= static_cast<uint32_t>(ConnectionEvent::READ);
		}
		if (fdEvent & (EPOLLOUT | EPOLLWRNORM))
		{
			Log().trace("Catch event EPOLLOUT on {} in ConnectionManager::wait()", connection->name());
			events |= static_cast<uint32_t>(ConnectionEvent::WRITE);
		}
		if (fdEvent & EPOLLHUP)
		{
			Log().trace("Catch event EPOLLEHUP on {} in ConnectionManager::wait()", connection->name());
			events |= static_cast<uint32_t>(ConnectionEvent::HUP);
		}
		if (fdEvent & EPOLLRDHUP)
		{
			Log().trace("Catch event EPOLLERDHUP on {} in ConnectionManager::wait()", connection->name());
			events |= static_cast<uint32_t>(ConnectionEvent::HALFHUP);
		}
		if (fdEvent & EPOLLERR)
		{
			Log().trace("Catch event EPOLLERR on {} in ConnectionManager::wait()", connection->name());
			events |= static_cast<uint32_t>(ConnectionEvent::ERROR);
		}

		connection->appendEvents(events);

		// Если не в списке захваченых...
		if (_capturedConnections.find(connection) == _capturedConnections.end())
		{
			Log().trace("Insert {} into ready connection list and will be processed now in ConnectionManager::wait()", connection->name());

			// ...добавляем в список готовых
			_readyConnections.insert(connection);

			Log().debug("Count of use {} after ConnectionManager::wait(): {}", connection->name(), connection.use_count());
		}
		else
		{
			Log().debug("{} found in captured connection list and will be processed later in ConnectionManager::wait()", connection->name());
		}
	}

//	Log().trace("End waiting in ConnectionManager::wait()");

	_epool_mutex.unlock();
}

/// Захватить соединение
ConnectionBase::Ptr ConnectionManager::capture()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	Log().trace("Try capture connection in ConnectionManager::capture()");

	// Если нет готовых...
	while (_readyConnections.empty())
	{
		Log().trace("Not found ready connection in ConnectionManager::capture()");

		// а в штатном режиме ожидаем появления готового соединения
		wait();
	}

	Log().trace("Found ready connection in ConnectionManager::capture()");

	if (_readyConnections.empty())
	{
		return nullptr;
	}

	// Берем соединение из набора готовых к обработке
	auto it = _readyConnections.begin();

	ConnectionBase::Ptr connection = *it;

//	Log().trace("Move {} from ready into captured connection list in ConnectionManager::captured()", connection->name());

	// Перемещаем соединение из набора готовых к обработке в набор захваченных
	_readyConnections.erase(it);
	_capturedConnections.insert(connection);

	Log().trace("Capture {} in ConnectionManager::capture()", connection->name());

	connection->setCaptured();

	return connection;
}

/// Освободить соединение
void ConnectionManager::release(ConnectionBase::Ptr connection)
{
//	Log().trace("Enter into ConnectionManager::release() for {}", connection->name());

	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	Log().trace("Remove {} from captured connection list in ConnectionManager::captured()", connection->name());

	_capturedConnections.erase(connection);

	Log().debug("Count of use {} after ConnectionManager::relese(): {}", connection->name(), connection.use_count());

	connection->setReleased();

	if (!connection->isClosed())
	{
		watch(connection);
	}
}

/// Обработка событий
void ConnectionManager::dispatch()
{
	Log().debug("Start dispatching in ConnectionManager::dispatch()");

	for (;;)
	{
		ConnectionBase::Ptr connection = getInstance().capture();

		if (!connection)
		{
			break;
		}

//		Log().trace("Enqueue {} into ThreadPool for procession ConnectionManager::dispatch()", connection->name());

		ThreadPool::enqueue([connection](){
			Log().trace("Begin processing for {}", connection->name());

			connection->processing();

			getInstance().release(connection);

			Log().debug("End processing for {}", connection->name());
		});
	}
}
