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
// File created on: 2017.10.07

// WebsocketCommunicator.cpp


#include <random>
#include "WebsocketCommunicator.hpp"
#include "../utils/SslHelper.hpp"
#include "../net/SslConnector.hpp"
#include "../net/ConnectionManager.hpp"
#include "../server/Server.hpp"
#include "../utils/Base64.hpp"
#include "../thread/RollbackStackAndRestoreContext.hpp"
#include "WebsocketPipe.hpp"

WebsocketCommunicator::WebsocketCommunicator(
	const HttpUri& uri,
	const std::shared_ptr<Transport::Handler>& handler
)
: Task(std::make_shared<Task::Func>([this](){return operator()();}))
, _log("WebsocketCommunicator")
, _websocketClient(std::make_shared<WebsocketClient>(handler))
, _uri(uri)
, _error("No run")
, _state(State::INIT)
{
	_log.debug("Created for address %s", _uri.str().c_str());
}

WebsocketCommunicator::~WebsocketCommunicator()
{
	_log.debug("Destroyed for address %s", _uri.str().c_str());
}

bool WebsocketCommunicator::operator()()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::INIT)
	{
		_log.warn("Bad step: connect");
		return false;
	}

	_error.clear();
	_log.trace("--------------------------------------------------------------------------------------------------------");
	_log.trace("Connect...");

	try
	{
		_state = State::CONNECT;

		if (_uri.scheme() == HttpUri::Scheme::HTTPS)
		{
			auto context = SslHelper::getClientContext();
			_connector = std::make_shared<SslConnector>(_websocketClient, _uri.host(), _uri.port(), context);
		}
		else
		{
			_connector = std::make_shared<TcpConnector>(_websocketClient, _uri.host(), _uri.port());
		}
		_connector->setTtl(std::chrono::seconds(15));

		_connector->addConnectedHandler(
			[wp = std::weak_ptr<Task>(ptr())]
			(const std::shared_ptr<TcpConnection>& connection)
			{
				auto iam = std::dynamic_pointer_cast<WebsocketCommunicator>(wp.lock());
				if (iam)
				{
					iam->_connector.reset();
					iam->_connection = connection;
					iam->onConnected();
				}
			}
		);

		_connector->addErrorHandler(
			[wp = std::weak_ptr<Task>(ptr())]
			()
			{
				auto iam = std::dynamic_pointer_cast<WebsocketCommunicator>(wp.lock());
				if (iam)
				{
					iam->_connector.reset();
					iam->failConnect();
				}
			}
		);

		ConnectionManager::add(_connector);
	}
	catch (const std::exception& exception)
	{
		_error = "Internal error: Uncatched exception at connect ← ";
		_error += exception.what();

		exceptionAtConnect();
	}

	return true;
}

void WebsocketCommunicator::failConnect()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		_log.warn("Bad step: failConnect");
		return;
	}

	_log.trace("Fail connect");

	_state = State::ERROR;

	done();
}

void WebsocketCommunicator::exceptionAtConnect()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		_log.warn("Bad step: exceptionConnect");
		return;
	}

	_log.trace("Exception at connect");

	_state = State::ERROR;

	done();
}

void WebsocketCommunicator::onConnected()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		_log.warn("Bad step: connected");
		return;
	}

	_log.trace("Connected");

	_state = State::CONNECTED;

	if (!_connection->getContext())
	{
		_connection->setContext(std::make_shared<WsContext>(_connection));
	}
	auto context = std::dynamic_pointer_cast<WsContext>(_connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context");
	}

	context->addEstablishedHandler(
		[wp = std::weak_ptr<Task>(ptr())]
		(WsContext&)
		{
			auto iam = std::dynamic_pointer_cast<WebsocketCommunicator>(wp.lock());
			if (iam)
			{
				iam->onComplete();
			}
		}
	);

	_connection->addErrorHandler(
		[wp = std::weak_ptr<Task>(ptr())]
		(TcpConnection&)
		{
			auto iam = std::dynamic_pointer_cast<WebsocketCommunicator>(wp.lock());
			if (iam)
			{
				iam->failProcessing();
			}
		}
	);

	submit();
}

void WebsocketCommunicator::submit()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECTED)
	{
		_log.warn("Bad step: submit");
		return;
	}

	_log.trace("Submit...");

	_state = State::SUBMIT;

	try
	{
		auto context = std::dynamic_pointer_cast<WsContext>(_connection->getContext());
		if (!context)
		{
			throw std::runtime_error("Bad context");
		}

		std::ostringstream oss;
		oss << "GET " << _uri.path() << (_uri.hasQuery() ? "?" : "") << (_uri.hasQuery() ? _uri.query() : "") << " HTTP/1.1\r\n"
			<< "Host: " << _uri.host() << ":" << _uri.port() << "\r\n"
			<< "User-Agent: " << Server::httpName() << "\r\n"
			<< "Pragma: no-cache\r\n"
			<< "Cache-Control: no-cache\r\n"
			<< "Connection: Upgrade\r\n"
			<< "Upgrade: websocket\r\n"
			<< "Sec-WebSocket-Version: 13\r\n"
			<< "Sec-WebSocket-Key: " << context->key() << "\r\n"
			<< "\r\n";

//		_log.debug("REQUEST: %s", _uri.str().c_str());

		_connection->write(oss.str().c_str(), oss.str().length());
		_connection->setTtl(std::chrono::milliseconds(5000));

		_log.trace("Submited");

		_state = State::SUBMITED;

		ConnectionManager::watch(_connection);
	}
	catch (const std::exception& exception)
	{
		_error = "Internal error: Uncatched exception at submit ← ";
		_error += exception.what();

		exceptionAtSubmit();
	}
}

void WebsocketCommunicator::exceptionAtSubmit()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::SUBMIT)
	{
		_log.warn("Bad step: exceptionSubmit");
		return;
	}

	_log.trace("Exception at submit");

	_state = State::ERROR;

	done();
}

void WebsocketCommunicator::failProcessing()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (
		_state != State::CONNECTED &&
		_state != State::SUBMIT &&
		_state != State::SUBMITED
		)
	{
		_log.warn("Bad step: failProcessing");
		return;
	}

	_log.trace("Error after connected");

	_state = State::ERROR;

	done();
}

void WebsocketCommunicator::onComplete()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::SUBMITED)
	{
		_log.warn("Bad step: complete");
		return;
	}

	_connection->setTtl(std::chrono::seconds(10));

	_error.clear();

	_log.trace("Established");
	_state = State::ESTABLISHED;

	done();
}

void WebsocketCommunicator::onError()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (
		_state != State::CONNECTED &&
		_state != State::SUBMIT &&
		_state != State::SUBMITED
		)
	{
		_log.warn("Bad step: failProcessing");
		return;
	}

	_log.trace("Error at processing");

	_state = State::ERROR;

	_connection->setTtl(std::chrono::milliseconds(50));

	done();
}

void WebsocketCommunicator::done()
{
	_log.trace("Cleanup...");

	if (_state != State::ESTABLISHED)
	{
		_connection.reset();
	}

	_connector.reset();

	_log.trace("Done");

	throw RollbackStackAndRestoreContext(ptr());
}
