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

// Request.hpp


#pragma once


#include <string>
#include <ucontext.h>
#include <sstream>
#include "ClientTransport.hpp"
#include "../net/TcpConnector.hpp"
#include "../net/ConnectionManager.hpp"
#include "../thread/Task.hpp"
#include "HttpClient.hpp"
#include "http/HttpContext.hpp"
#include "../net/SslConnector.hpp"
#include "../utils/SslHelper.hpp"

class Request: public Task
{
private:
	std::shared_ptr<ClientTransport> _clientTransport;
	HttpUri _uri;
	std::string _answer;
	std::string _error;

	enum class State
	{
		INIT,
		CONNECT_IN_PROGRESS,
		CONNECTED,
		WRITE,
		READ,
		DONE,
		ERROR
	} _state;

public:
	Request(HttpUri uri, HttpRequest::Method method = HttpRequest::Method::GET, std::string body = std::string())
	: Task([this](){
		return operator()();
	})
	, _clientTransport(new HttpClient())
	, _uri(std::move(uri))
	, _state(State::INIT)
	{
	};
	~Request() {};

	const std::string& answer() const
	{
		return _answer;
	}
	const std::string& error() const
	{
		return _error;
	}
	const bool hasFailed() const
	{
		return !_error.empty();
	}

	virtual bool operator()()
	{
		switch (_state)
		{
			case State::INIT:
				try
				{
					Log("_").debug("connector in progress");
					_state = State::CONNECT_IN_PROGRESS;
					std::shared_ptr<TcpConnector> connector;
					if (_uri.scheme() == HttpUri::Scheme::HTTPS)
					{
						auto context = SslHelper::context();
						connector.reset(new SslConnector(_clientTransport, _uri.host(), _uri.port(), context));
					}
					else
					{
						connector.reset(new TcpConnector(_clientTransport, _uri.host(), _uri.port()));
					}
					connector->addConnectedHandler([this](std::shared_ptr<TcpConnection> connection){
						Log("_").debug("connected");
						connection->addCompleteHandler([this](const std::shared_ptr<Context>&context_){
							Log("_").debug("done");

							auto context = std::dynamic_pointer_cast<HttpContext>(context_);
							if (!context)
							{
								_error = "Internal error: Bad context";
								_state = State::ERROR;
								operator()();
								return;
							}

							if (!context->getResponse())
							{
								_error = "Internal error: No response";
								_state = State::ERROR;
								operator()();
								return;
							}

							_answer = std::move(std::string(context->getResponse()->dataPtr(), context->getResponse()->dataLen()));

							if (context->getResponse()->statusCode() != 200)
							{
								_error = std::string("No OK response: ") + std::to_string(context->getResponse()->statusCode()) + " " + context->getResponse()->statusMessage();
								_state = State::ERROR;
								operator()();
								return;
							}

							_state = State::DONE;
							operator()();
						});

						connection->addErrorHandler([this](){
							Log("_").debug("connection error");
							_state = State::ERROR;
							operator()();
						});

						std::ostringstream oss;
						oss << "GET " << _uri.path() << (_uri.hasQuery() ? _uri.query() : "") << " HTTP/1.1\r\n"
							<< "Host: " << _uri.host() << ":" << _uri.port() << "\r\n"
							<< "Connection: Close\r\n"
							<< "\r\n";

						connection->write(oss.str().c_str(), oss.str().length());
						ConnectionManager::watch(connection);
						operator()();
					});
					connector->addErrorHandler([this](){
						Log("_").debug("connector error");
						_state = State::ERROR;
						operator()();
					});
					Log("_").debug("add connector");
					ConnectionManager::add(connector);
				}
				catch (const std::exception& exception)
				{
					Log("_").debug("connector exception");
					_error = "Internal error: Uncatched exception: ";
					_error += exception.what();
					_state = State::ERROR;
					operator()();
				}
				break;

			case State::CONNECT_IN_PROGRESS:
				break;

			case State::CONNECTED:
				break;

			case State::WRITE:
				break;

			case State::READ:
				break;

			case State::DONE:
			case State::ERROR:
				restoreContext();
				return true;
		}

		return false;
	};
};
