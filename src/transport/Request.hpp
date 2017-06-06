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

class Request: public Task
{
private:
	std::shared_ptr<ClientTransport> _clientTransport;
	std::string _host;
	int _port;
	std::string _answer;

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
	Request(std::string host, int port)
	: Task([this](){
		return operator()();
	})
	, _clientTransport(new ClientTransport())
	, _host(host)
	, _port(port)
	, _state(State::INIT)
	{
	};
	~Request() {};

	virtual bool operator()()
	{
		switch (_state)
		{
			case State::INIT:
				try
				{
					Log("_").debug("connector in progress");
					_state = State::CONNECT_IN_PROGRESS;
					auto connector = std::make_shared<TcpConnector>(_clientTransport, _host, _port);
					connector->addConnectedHandler([this](std::shared_ptr<TcpConnection> connection){
						Log("_").debug("connected");
						_state = State::CONNECTED;
						connection->addCompleteHandler([this](){
							Log("_").debug("done");
							_state = State::DONE;
							operator()();
						});

						connection->addErrorHandler([this](){
							Log("_").debug("connection error");
							_state = State::ERROR;
							operator()();
						});

						std::ostringstream oss;
						oss << "GET /oauth/access_token HTTP/1.1\r\n"
							<< "Host: " << _host << ":" << _port << "\r\n"
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
				catch (...)
				{
					Log("_").debug("connector exception");
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
