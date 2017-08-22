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

// Request.cpp


#include "Request.hpp"

Request::Request(const HttpUri& uri, HttpRequest::Method method, const std::string& body)
: Task(std::make_shared<Task::Func>([this](){return operator()();}))
, _log("Request")
, _clientTransport(new HttpClient())
, _method(method)
, _uri(uri)
, _body(body)
, _error("Not executed")
, _state(State::INIT)
{
}

bool Request::execute()
{
	switch (_state)
	{
		case State::INIT:
			_log.trace("Request::execute INIT");
			return this->connect();

		case State::CONNECT_IN_PROGRESS:
			_log.trace("Request::execute CONNECT_IN_PROGRESS");
			break;

		case State::CONNECTED:
			_log.trace("Request::execute CONNECTED");
			break;

		case State::WRITE:
			_log.trace("Request::execute WRITE");
			break;

		case State::READ:
			_log.trace("Request::execute READ");
			break;

		case State::DONE:
		case State::ERROR:
		{
			Log log("Request");
			log.debug("REQUEST: %s", _uri.str().c_str());
			if (!_answer.empty())
			{
				log.debug("RESPONSE: %s", _answer.c_str());
			}
			if (!_error.empty())
			{
				log.debug("ERROR: %s", _error.c_str());
			}

			restoreContext();
			return true;
		}
	}

	return false;
}

bool Request::connect()
{
	_log.trace("Request::execute connect");

	try
	{
		_log.debug("connector in progress");
		_state = State::CONNECT_IN_PROGRESS;
		std::shared_ptr<TcpConnector> connector;
		if (_uri.scheme() == HttpUri::Scheme::HTTPS)
		{
			auto context = SslHelper::getClientContext();
			connector.reset(new SslConnector(_clientTransport, _uri.host(), _uri.port(), context));
		}
		else
		{
			connector.reset(new TcpConnector(_clientTransport, _uri.host(), _uri.port()));
		}
		connector->setTtl(std::chrono::seconds(15));
		connector->addConnectedHandler([this](const std::shared_ptr<TcpConnection>& connection){
			_log.debug("connected");
			_state = State::CONNECTED;
			connection->addCompleteHandler([this,connection](const std::shared_ptr<Context>&context_){
				_log.debug("done");

				auto context = std::dynamic_pointer_cast<HttpContext>(context_);
				if (!context)
				{
					_error = "Internal error: Bad context";
					_state = State::ERROR;
					connection->setTtl(std::chrono::milliseconds(50));
					connection->close();
					operator()();
					return;
				}

				if (!context->getResponse())
				{
					_error = "Internal error: No response";
					_state = State::ERROR;
					connection->setTtl(std::chrono::milliseconds(50));
					connection->close();
					operator()();
					return;
				}

				_answer = std::move(std::string(context->getResponse()->dataPtr(), context->getResponse()->dataLen()));

				if (context->getResponse()->statusCode() != 200)
				{
					_error = std::string("No OK response: ") + std::to_string(context->getResponse()->statusCode()) + " " + context->getResponse()->statusMessage();
					_state = State::ERROR;
					connection->setTtl(std::chrono::milliseconds(50));
					connection->close();
					operator()();
					return;
				}

				_state = State::DONE;
				connection->setTtl(std::chrono::milliseconds(50));
				connection->close();
				operator()();
			});

			connection->addErrorHandler([this,connection](){
				_log.debug("connection error");
				_state = State::ERROR;
				connection->setTtl(std::chrono::milliseconds(50));
				connection->close();
				operator()();
			});

			std::ostringstream oss;
			oss << (
				_method == HttpRequest::Method::GET ? "GET " :
				_method == HttpRequest::Method::POST ? "POST " :
				"UNKNOWN "
			) << _uri.path() << (_uri.hasQuery() ? "?" : "") << (_uri.hasQuery() ? _uri.query() : "") << " HTTP/1.1\r\n"
				<< "Host: " << _uri.host() << ":" << _uri.port() << "\r\n"
				<< "Connection: Close\r\n";
			if (_method == HttpRequest::Method::POST)
			{
				oss << "Content-Length: " << _body.length() << "\r\n";
			}
			oss	<< "\r\n";
			if (_method == HttpRequest::Method::POST)
			{
				oss << _body;
			}

//			Log("Request", detail).debug("REQUEST: %s", oss.str().c_str());

			connection->write(oss.str().c_str(), oss.str().length());
			connection->setTtl(std::chrono::milliseconds(5000));
			ConnectionManager::watch(connection);
			operator()();
		});
		connector->addErrorHandler([this,connector](){
			_log.debug("connector error");
			_state = State::ERROR;
			operator()();
		});
		_log.debug("add connector");
		ConnectionManager::add(connector);
	}
	catch (const std::exception& exception)
	{
		_log.debug("connector exception");
		_error = "Internal error: Uncatched exception ← ";
		_error += exception.what();
		_state = State::ERROR;
		operator()();
	}

	return true;
}
