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

// RequestBase.cpp


#include "RequestBase.hpp"

RequestBase::RequestBase(Connection::Ptr& connection, const void *data)
: _data(data)
{
	_name = nullptr;

//	if (_session)
//	{
//		_session->lock("ActionBase", __LINE__);
//		_session->setRequestId(_requestId);
//
//		if (_session->isInactive(Config::timeoutForActive()) && needTouchSession())
//		{
//			_session->logIn();
//		}
//		_session->unlock("ActionBase", __LINE__);
//	}
}

RequestBase::~RequestBase()
{
}

const char *RequestBase::getName() const
{
	return _name ? _name : (_name = "request");
}
