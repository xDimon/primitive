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
// File created on: 2017.02.26

// Transport.hpp


#pragma once

#include <memory>
#include "../log/Log.hpp"

class Connection;

class Transport: public virtual Log
{
public:
	typedef std::shared_ptr<Transport> Ptr;
	typedef std::weak_ptr<Transport> WPtr;

private:
	std::unique_ptr<std::function<std::shared_ptr<Connection>()>> _acceptorCreator;
	std::weak_ptr<Connection> _acceptor;

public:
	template<class F, class... Args>
	Transport(F &&f, Args &&... args)
	{
		_acceptorCreator = std::make_unique<std::function<std::shared_ptr<Connection>()>>(
			std::bind(std::forward<F>(f), std::shared_ptr<Transport>(this), std::forward<Args>(args)...)
		);
	}

	virtual bool enable();
	virtual bool disable();

	virtual bool processing(std::shared_ptr<Connection> connection) = 0;
};
