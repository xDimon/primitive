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

// WsTransport.hpp


#pragma once

#include "Transport.hpp"

class WsTransport : public Transport
{
public:
	template<class F, class... Args>
	WsTransport(F &&f, Args &&... args)
	: Log("WsTransport")
	, Transport(f, args...)
	{
		log().debug("Create '%s'", name().c_str());
	}
	virtual ~WsTransport()
	{
		log().debug("Destroy '%s'", name().c_str());
	}

	virtual bool processing(std::shared_ptr<Connection> connection);
};
