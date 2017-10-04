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
// File created on: 2017.03.29

// WsContext.hpp


#pragma once

#include "../http/HttpContext.hpp"
#include "WsFrame.hpp"
#include "../../sessions/Session.hpp"

class WsContext : public HttpContext
{
protected:
	bool _established;
	std::shared_ptr<WsFrame> _frame;
	std::weak_ptr<Session> _session;

public:
	WsContext()
	: _established(false)
	{};

	virtual ~WsContext() = default;

	void setEstablished()
	{
		_established = true;
		_frame.reset();
		_request.reset();
		_response.reset();
	}
	bool established()
	{
		return _established;
	}

	const std::shared_ptr<WsFrame>& getFrame()
	{
		return _frame;
	}
	void setFrame(const std::shared_ptr<WsFrame>& frame)
	{
		_frame = frame;
	}
	void resetFrame()
	{
		_frame.reset();
	}

	void assignSession(const std::shared_ptr<Session>& session)
	{
		_session = session;
	}
	std::shared_ptr<Session> getSession()
	{
		return _session.lock();
	}
	void resetSession()
	{
		_session.reset();
	}
};
