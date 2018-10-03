// Copyright © 2017-2018 Dmitriy Khaustov
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
// File created on: 2017.06.06

// HttpRequestExecutor.cpp


#include "HttpRequestExecutor.hpp"
#include "../../utils/SslHelper.hpp"
#include "../../net/SslConnector.hpp"
#include "../../net/ConnectionManager.hpp"
#include "HttpContext.hpp"
#include "../../thread/Thread.hpp"
#include "../../thread/ThreadPool.hpp"
#include "../../thread/RollbackStackAndRestoreContext.hpp"

HttpRequestExecutor::HttpRequestExecutor(
	const HttpUri& uri,
	HttpRequest::Method method,
	const std::string& body,
	const std::string& contentType
)
: _savedCtx(nullptr)
, _log("HttpRequestExecutor")
, _clientTransport(new HttpClient())
, _method(method)
, _uri(uri)
, _body(body)
, _contentType(contentType)
, _error("No run")
, _state(State::INIT)
{
//	_log.setDetail(Log::Detail::TRACE);
}

HttpRequestExecutor::~HttpRequestExecutor()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::ERROR && _state != State::COMPLETE)
	{
		_log.warn("Destruct when step: %d", static_cast<int>(_state));
	}
}

void HttpRequestExecutor::operator()()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state != State::INIT)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + ", but expected "
			"INIT(" + std::to_string(static_cast<int>(State::INIT)) + ")");
		return;
	}

	if (!Thread::getCurrTaskContext())
	{
		_log.trace("Thread::_currentTaskContextPtrBuffer undefined");
//		throw std::runtime_error("Thread::_currentTaskContextPtrBuffer undefined");
	}
	_savedCtx = Thread::getCurrTaskContext();
	Thread::setCurrTaskContext(nullptr);

	_error.clear();
	_log.trace("--------------------------------------------------------------------------------------------------------");
	_log.trace("Connect...");

	try
	{
		_state = State::CONNECT;

		if (_uri.scheme() == HttpUri::Scheme::HTTPS)
		{
			auto context = SslHelper::getClientContext();
			_connector = std::make_shared<SslConnector>(_clientTransport, _uri.host(), _uri.port(), context);
		}
		else
		{
			_connector = std::make_shared<TcpConnector>(_clientTransport, _uri.host(), _uri.port());
		}
		_connector->setTtl(std::chrono::seconds(15));

		_connector->addConnectedHandler(
			[wp = std::weak_ptr<HttpRequestExecutor>(ptr())]
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
			[wp = std::weak_ptr<HttpRequestExecutor>(ptr())]
			{
				auto iam = std::dynamic_pointer_cast<HttpRequestExecutor>(wp.lock());
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

void HttpRequestExecutor::failConnect()
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

void HttpRequestExecutor::exceptionAtConnect()
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

void HttpRequestExecutor::onConnected()
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
		_connection->setContext(std::make_shared<HttpContext>(_connection));
	}
	auto context = std::dynamic_pointer_cast<HttpContext>(_connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context");
	}

	_connection->addCompleteHandler(
		[wp = std::weak_ptr<HttpRequestExecutor>(ptr())]
		(TcpConnection&, const std::shared_ptr<Context>&)
		{
			auto iam = wp.lock();
			if (iam)
			{
				iam->onComplete();
			}
		}
	);

	_connection->addErrorHandler(
		[wp = std::weak_ptr<HttpRequestExecutor>(ptr())]
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

void HttpRequestExecutor::submit()
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
		std::ostringstream oss;
		oss << (
			_method == HttpRequest::Method::GET
			? "GET "
			: _method == HttpRequest::Method::POST
			  ? "POST "
			  : "UNKNOWN "
		) << _uri.path() << (_uri.hasQuery() ? "?" : "") << (_uri.hasQuery() ? _uri.query() : "") << " HTTP/1.1\r\n"
			<< "Host: " << _uri.host() << ":" << _uri.port() << "\r\n"
			<< "Connection: Close\r\n";
		if (_method == HttpRequest::Method::POST)
		{
			oss << "Content-Length: " << _body.length() << "\r\n";
		}
		if (_method == HttpRequest::Method::POST)
		{
			if (!_contentType.empty())
			{
				oss << "Content-Type: " << _contentType << "\r\n";
			}
			oss << "\r\n";
			oss << _body;
		}
		else
		{
			oss << "\r\n";
		}

//		_log.debug("REQUEST: %s", _uri.str().c_str());

		_connection->write(oss.str().c_str(), oss.str().length());
		_connection->setTtl(std::chrono::seconds(60));

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

void HttpRequestExecutor::exceptionAtSubmit()
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

void HttpRequestExecutor::failProcessing()
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

void HttpRequestExecutor::onComplete()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state == State::COMPLETE)
	{
		return;
	}
	if (_state != State::SUBMITED)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at onComplete(), but expected "
			"SUBMITED(" + std::to_string(static_cast<int>(State::SUBMITED)) + ")");
		return;
	}

	auto context = std::dynamic_pointer_cast<HttpContext>(_connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context");
	}

	if (!context->getResponse())
	{
		_error = "Complete with error (No response)";
		onError();
		return;
	}

	_answer = std::string(context->getResponse()->dataPtr(), context->getResponse()->dataLen());

	if (context->getResponse()->statusCode() != 200)
	{
		_error = "No OK response: " + std::to_string(context->getResponse()->statusCode()) + " " + context->getResponse()->statusMessage();
		onError();
		return;
	}

	_error.clear();

	_log.trace("Completed");
	_state = State::COMPLETE;

	_connection->setTtl(std::chrono::milliseconds(50));

	done();
}

void HttpRequestExecutor::onError()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (
		_state != State::CONNECTED &&
		_state != State::SUBMIT &&
		_state != State::SUBMITED
	)
	{
		badStep("Step " + std::to_string(static_cast<int>(_state)) + " at onError(), but expected "
			"CONNECTED(" + std::to_string(static_cast<int>(State::CONNECTED)) + "), "
			"SUBMIT(" + std::to_string(static_cast<int>(State::SUBMIT)) + "), "
			"SUBMITED(" + std::to_string(static_cast<int>(State::SUBMITED)) + ")");
		return;
	}

	_log.trace("Error at processing");

	_state = State::ERROR;
	if (_error.empty())
	{
		_error = "Error at processing";
	}

	_connection->setTtl(std::chrono::milliseconds(50));

	done();
}

void HttpRequestExecutor::badStep(const std::string& msg)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (_state == State::ERROR || _state == State::COMPLETE)
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

void HttpRequestExecutor::done()
{
	ucontext_t* ctx = nullptr;

	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

		_log.trace("Cleanup...");

		_connection.reset();
		_connector.reset();

		ctx = _savedCtx;
		_savedCtx = nullptr;

		_log.trace("Done");
	}

	throw RollbackStackAndRestoreContext(ctx);
}
