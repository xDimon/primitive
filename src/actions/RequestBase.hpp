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
// File created on: 2017.04.06

// RequestBase.hpp


#pragma once

#include <cstdint>
#include "../net/Connection.hpp"
#include "../sessions/Session.hpp"

class RequestBase
{
private:
	mutable const char *_name;
	const void *_data;

protected:
	Connection::Ptr _connection;
	Session::Ptr _session;

public:
	RequestBase() = delete;
	RequestBase(const RequestBase&) = delete;
	void operator= (RequestBase const&) = delete;

	RequestBase(Connection::Ptr& connection, const void *data);
	virtual ~RequestBase();

	const char *getName() const;

	inline Session::Ptr getSession() const
	{
		return _session;
	}

	virtual bool validate() = 0;
	virtual bool execute() = 0;

	virtual bool isCanBeFirst() const
	{
		return false;
	}

	virtual bool needTouchSession() const
	{
		return true;
	}
};

#include "ActionsFactory.hpp"

#define REGISTER_REQUEST(Request) const std::string Request::__dummy_for_reg_call = ActionsFactory::regRequest(#Request, Request::create);
#define DECLARE_REQUEST(Request) \
private:																						\
	Request() = delete;																			\
	Request(const Request&) = delete;															\
	void operator= (Request const&) = delete;													\
																								\
	Request(Connection::Ptr& connection, const void *request);								\
																								\
public:																							\
	virtual ~Request() {};																		\
																								\
	virtual bool validate();																	\
	virtual bool execute();																		\
																								\
private:																						\
	static RequestBase *create(Connection::Ptr& connection, const void *request)			\
	{																							\
		return new Request(connection, request);												\
	}																							\
	static const std::string __dummy_for_reg_call;
