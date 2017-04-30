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
#include <cstring>
#include "../thread/ThreadPool.hpp"
#include "TcpConnection.hpp"
#include <unistd.h>

ConnectionManager::ConnectionManager()
: Log("ConnectionManager")
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
void ConnectionManager::add(Connection::Ptr connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	if (getInstance()._aConns.find(connection) != getInstance()._aConns.end())
	{
		getInstance().log().debug("Fail of add %s in ConnectionManager::add()", connection->name().c_str());
		return;
	}

	getInstance().log().debug("Add %s in ConnectionManager::add()", connection->name().c_str());

	getInstance()._aConns.insert(connection);

	epoll_event ev;

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_ADD, connection->fd(), &ev) == -1)
	{
		getInstance().log().debug("Fail call `epoll_ctl(ADD)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}
}

/// Удалить регистрацию соединения
bool ConnectionManager::remove(Connection::Ptr connection)
{
	if (!connection)
	{
		return false;
	}

	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

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
		getInstance().log().debug("Fail call `epoll_ctl(DEL)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}

	getInstance()._readyConnections.erase(connection);
	getInstance()._capturedConnections.erase(connection);

	return true;
}

uint32_t ConnectionManager::rotateEvents(Connection::Ptr connection)
{
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);
	uint32_t events = connection->rotateEvents();
	return events;
}

void ConnectionManager::watch(Connection::Ptr connection)
{
	// Для известных соенинений проверяем состояние захваченности
	std::lock_guard<std::recursive_mutex> guard(getInstance()._mutex);

	// Те, что в обработке, не трогаем
	if (connection->isCaptured())
	{
//		log().trace("Skip watch for %s in ConnectionManager::watch() because already captured", connection->name().c_str());
		return;
	}

//	log().trace("Watch for %s in ConnectionManager::watch()", connection->name().c_str());

	epoll_event ev;

	connection->watch(ev);

	// Включаем наблюдение
	if (epoll_ctl(getInstance()._epfd, EPOLL_CTL_MOD, connection->fd(), &ev) == -1)
	{
		getInstance().log().debug("Fail call `epoll_ctl(MOD)` for %s (error: '%s') in ConnectionManager::add()", connection->name().c_str(), strerror(errno));
	}
}

/// Ожидать события на соединениях
void ConnectionManager::wait()
{
	// Если набор готовых соединений не пустой, то выходим
	if (!_readyConnections.empty())
	{
//		log().trace("No wait in ConnectionManager::wait() because have ready connection");
		return;
	}

	_mutex.unlock();

	_epool_mutex.lock();

//	log().trace("Begin waiting in ConnectionManager::wait()");

	int n;

	for (;;)
	{
		n = epoll_wait(_epfd, _epev, poolSize, 50);
		if (n < 0)
		{
			if (errno != EINTR)
			{
				log().warn("epoll_wait error (%s) in ConnectionManager::wait()", strerror(errno));
			}
			continue;
		}
		if (n > 0)
		{
			log().trace_("Catch event(s) on %d connection(s) in ConnectionManager::wait()", n);
			break;
		}
	}

	_mutex.lock();

	// Перебираем полученые события
	for (int i = 0; i < n; i++)
	{
		Connection::Ptr connection = std::move(*static_cast<std::shared_ptr<Connection> *>(_epev[i].data.ptr));

		if (!connection)
		{
			log().trace_("Skip nullptr in ConnectionManager::wait(): %p", _epev[i].data.ptr);
			continue;
		}

		log().trace_("Catch event(s) `%d` on %s in ConnectionManager::wait()", _epev[i].events, connection->name().c_str());

		// Игнорируем незарегистрированные соединения
		if (_aConns.find(connection) == _aConns.end())
		{
			log().warn("Skip %s in ConnectionManager::wait() because connection unregistered", connection->name().c_str());
			continue;
		}

		uint32_t fdEvent = _epev[i].events;

		uint32_t events = 0;
		if (fdEvent & (EPOLLIN | EPOLLRDNORM))
		{
			log().trace_("Catch event EPOLLIN on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::READ);
		}
		if (fdEvent & (EPOLLOUT | EPOLLWRNORM))
		{
			log().trace_("Catch event EPOLLOUT on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::WRITE);
		}
		if (fdEvent & EPOLLHUP)
		{
			log().trace_("Catch event EPOLLEHUP on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::HUP);
		}
		if (fdEvent & EPOLLRDHUP)
		{
			log().trace_("Catch event EPOLLERDHUP on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::HALFHUP);
		}
		if (fdEvent & EPOLLERR)
		{
			log().trace_("Catch event EPOLLERR on %s in ConnectionManager::wait()", connection->name().c_str());
			events |= static_cast<uint32_t>(ConnectionEvent::ERROR);
		}

		connection->appendEvents(events);

		// Если не в списке захваченых...
		if (_capturedConnections.find(connection) == _capturedConnections.end())
		{
			log().trace_(
				"Insert %s into ready connection list and will be processed now in ConnectionManager::wait()", connection->name().c_str());

			// ...добавляем в список готовых
			_readyConnections.insert(connection);
		}
	}

//	log().trace("End waiting in ConnectionManager::wait()");

	_epool_mutex.unlock();
}

/// Захватить соединение
Connection::Ptr ConnectionManager::capture()
{
	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	log().trace("Try capture connection in ConnectionManager::capture()");

	// Если нет готовых...
	while (_readyConnections.empty())
	{
		log().trace_("Not found ready connection in ConnectionManager::capture()");

		// а в штатном режиме ожидаем появления готового соединения
		wait();
	}

	log().trace_("Found ready connection in ConnectionManager::capture()");

	if (_readyConnections.empty())
	{
		return nullptr;
	}

	// Берем соединение из набора готовых к обработке
	auto it = _readyConnections.begin();

	auto connection = *it;

//	log().trace("Move %s from ready into captured connection list in ConnectionManager::captured()", connection->name().c_str());

	// Перемещаем соединение из набора готовых к обработке в набор захваченных
	_readyConnections.erase(it);
	_capturedConnections.insert(connection);

	log().trace_("Capture %s in ConnectionManager::capture()", connection->name().c_str());

	connection->setCaptured();

	return connection;
}

/// Освободить соединение
void ConnectionManager::release(Connection::Ptr connection)
{
//	log().trace("Enter into ConnectionManager::release() for %s", connection->name().c_str());

	std::lock_guard<std::recursive_mutex> guard(_mutex);

//	log().trace("Remove %s from captured connection list in ConnectionManager::captured()", connection->name().c_str());

	_capturedConnections.erase(connection);

//	log().debug("In ConnectionManager::relese()");

	connection->setReleased();

	if (!connection->isClosed())
	{
//		log().debug("Before call watch in ConnectionManager::relese()");

		watch(connection);
	}
}

/// Обработка событий
void ConnectionManager::dispatch()
{
	getInstance().log().debug("Start dispatching in ConnectionManager::dispatch()");

	for (;;)
	{
//		log().debug("Before call capture in ConnectionManager::dispatch()");

		Connection::Ptr connection = getInstance().capture();

//		log().debug("After call capture in ConnectionManager::dispatch()");

		if (!connection)
		{
			break;
		}

//		log().trace("Enqueue %s into ThreadPool for procession ConnectionManager::dispatch()", connection->name().c_str());

		ThreadPool::enqueue([connection](){
			getInstance().log().trace_("Begin processing for %s", connection->name().c_str());

			connection->processing();

			getInstance().release(connection);

			getInstance().log().trace_("End processing for %s", connection->name().c_str());
		});
	}
}
