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
// File created on: 2017.10.07

// WsCommunicator.cpp


#include <random>
#include "WsCommunicator.hpp"
#include "../../utils/SslHelper.hpp"
#include "../../net/SslConnector.hpp"
#include "../../net/ConnectionManager.hpp"
#include "../../server/Server.hpp"
#include "../../thread/RollbackStackAndRestoreContext.hpp"
#include "WsContext.hpp"

WsCommunicator::WsCommunicator(
	const HttpUri& uri,
	const std::shared_ptr<Transport::Handler>& handler
)
: _savedCtx(nullptr)
, _log("WsCommunicator")
, _websocketClient(std::make_shared<WsClient>(handler))
, _uri(uri)
, _error("No run")
, _state(State::INIT)
{
	_log.debug("Created for address %s", _uri.str().c_str());
}

WsCommunicator::~WsCommunicator()
{
	_log.debug("Destroyed for address %s", _uri.str().c_str());
}

void WsCommunicator::operator()()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::INIT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + ", but expected "
			"INIT(" + std::to_string(static_cast<int>(State::INIT)) + ")");
		return;
	}

	_savedCtx = Thread::_currentTaskContextPtrBuffer;
	Thread::_currentTaskContextPtrBuffer = nullptr;

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
			[wp = std::weak_ptr<WsCommunicator>(ptr())]
			(const std::shared_ptr<TcpConnection>& connection)
			{
				auto iam = wp.lock();
				if (iam)
				{
					iam->_connector.reset();
					iam->_connection = connection;
					iam->onConnected();
				}
			}
		);

		_connector->addErrorHandler(
			[wp = std::weak_ptr<WsCommunicator>(ptr())]
			()
			{
				auto iam = wp.lock();
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
		_error = "Exception at connect ← ";
		_error += exception.what();

		exceptionAtConnect();
	}
}

void WsCommunicator::failConnect()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at failConnect(), but expected "
			"CONNECT (" + std::to_string(static_cast<int>(State::CONNECT)) + ")");
		return;
	}

	_log.trace("Fail connect");

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Fail connect";
	}

	done();
}

void WsCommunicator::exceptionAtConnect()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at exceptionConnect(), but expected "
			"CONNECT (" + std::to_string(static_cast<int>(State::CONNECT)) + ")");
		return;
	}

	_log.trace("Exception at connect");

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Exception at connect";
	}

	done();
}

void WsCommunicator::onConnected()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at onConnected(), but expected "
			"CONNECT(" + std::to_string(static_cast<int>(State::CONNECT)) + ")");
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
		[wp = std::weak_ptr<WsCommunicator>(ptr())]
		(WsContext&)
		{
			auto iam = wp.lock();
			if (iam)
			{
				iam->onComplete();
			}
		}
	);

	_connection->addErrorHandler(
		[wp = std::weak_ptr<WsCommunicator>(ptr())]
		(TcpConnection&)
		{
			auto iam = wp.lock();
			if (iam)
			{
				iam->failProcessing();
			}
		}
	);

	submit();
}

void WsCommunicator::submit()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::CONNECTED)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at submit(), but expected "
			"CONNECTED(" + std::to_string(static_cast<int>(State::CONNECTED)) + ")");
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

void WsCommunicator::exceptionAtSubmit()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::SUBMIT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at exceptionAtSubmit(), but expected "
			"SUBMIT(" + std::to_string(static_cast<int>(State::SUBMIT)) + ")");
		return;
	}

	_log.trace("Exception at submit");

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Exception at submit";
	}

	done();
}

void WsCommunicator::failProcessing()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (
		_state != State::CONNECTED &&
		_state != State::SUBMIT &&
		_state != State::SUBMITED
	)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at failProcessing(), but expected "
			"CONNECTED(" + std::to_string(static_cast<int>(State::CONNECTED)) + "), "
			"SUBMIT(" + std::to_string(static_cast<int>(State::SUBMIT)) + "), "
			"SUBMITED(" + std::to_string(static_cast<int>(State::SUBMITED)) + ")");
		return;
	}

	_log.trace("Error after connected");

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Exception after connected";
	}

	done();
}

void WsCommunicator::onComplete()
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

void WsCommunicator::onError()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (
		_state != State::CONNECTED &&
		_state != State::SUBMIT &&
		_state != State::SUBMITED
		)
	{
		_log.warn("Bad step: onError");
		return;
	}

	_log.trace("Error at processing");

	_state = State::ERROR;

	_connection->setTtl(std::chrono::milliseconds(50));

	done();
}

void WsCommunicator::badStep(const std::string& msg)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (_state == State::ERROR || _state == State::ESTABLISHED)
	{
		return;
	}

	_log.warn("Bad step: %s", msg.c_str());

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Processing order error";
	}

	done();
}

void WsCommunicator::done()
{
	_log.trace("Cleanup...");

	if (_state != State::ESTABLISHED)
	{
		_connection.reset();
	}

	_connector.reset();

	_log.trace("Done");

	throw RollbackStackAndRestoreContext(_savedCtx);
}
