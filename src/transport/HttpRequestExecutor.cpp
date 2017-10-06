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
// File created on: 2017.06.06

// HttpRequestExecutor.cpp


#include "HttpRequestExecutor.hpp"
#include "../thread/RollbackStackAndRestoreContext.hpp"
#include "http/HttpContext.hpp"

HttpRequestExecutor::HttpRequestExecutor(
	const HttpUri& uri,
	HttpRequest::Method method,
	const std::string& body,
	const std::string& contentType
)
: Task(std::make_shared<Task::Func>([this](){return operator()();}))
, _log("HttpRequestExecutor")
, _clientTransport(new HttpClient())
, _method(method)
, _uri(uri)
, _body(body)
, _contentType(contentType)
, _error("No run")
, _state(State::INIT)
{
}

bool HttpRequestExecutor::operator()()
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
			_connector = std::make_shared<SslConnector>(_clientTransport, _uri.host(), _uri.port(), context);
		}
		else
		{
			_connector = std::make_shared<TcpConnector>(_clientTransport, _uri.host(), _uri.port());
		}
		_connector->setTtl(std::chrono::seconds(15));

		_connector->addConnectedHandler(
			[wp = std::weak_ptr<Task>(ptr())]
			(const std::shared_ptr<TcpConnection>& connection)
			{
				auto iam = std::dynamic_pointer_cast<HttpRequestExecutor>(wp.lock());
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
		_error = "Internal error: Uncatched exception at connect ← ";
		_error += exception.what();

		exceptionAtConnect();
	}

	return true;
}

void HttpRequestExecutor::failConnect()
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

void HttpRequestExecutor::exceptionAtConnect()
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

void HttpRequestExecutor::onConnected()
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
		_connection->setContext(std::make_shared<HttpContext>(_connection));
	}
	auto context = std::dynamic_pointer_cast<HttpContext>(_connection->getContext());
	if (!context)
	{
		throw std::runtime_error("Bad context");
	}

	_connection->addCompleteHandler(
		[wp = std::weak_ptr<Task>(ptr())]
		(TcpConnection&, const std::shared_ptr<Context>&)
		{
			auto iam = std::dynamic_pointer_cast<HttpRequestExecutor>(wp.lock());
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
			auto iam = std::dynamic_pointer_cast<HttpRequestExecutor>(wp.lock());
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
		_log.warn("Bad step: submit");
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

void HttpRequestExecutor::exceptionAtSubmit()
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

void HttpRequestExecutor::failProcessing()
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

void HttpRequestExecutor::onComplete()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
	if (_state == State::COMPLETE)
	{
		return;
	}
	if (_state != State::SUBMITED)
	{
		_log.warn("Bad step: complete");
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
		_log.warn("Bad step: failProcessing");
		return;
	}

	_log.trace("Error at processing");

	_state = State::ERROR;

	_connection->setTtl(std::chrono::milliseconds(50));

	done();
}

void HttpRequestExecutor::done()
{
	_log.trace("Cleanup...");

	_connection.reset();
	_connector.reset();

	_log.trace("Done");

	throw RollbackStackAndRestoreContext(ptr());
}