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
// File created on: 2017.02.26

// Transport.hpp


#pragma once


#include <functional>
#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"
#include "../utils/Context.hpp"
#include "../log/Log.hpp"

class Connection;

class Transport : public Named
{
protected:
	Log _log;

public:
	typedef std::function<void(const char*, size_t, const std::string&, bool)> Transmitter;
	typedef std::function<void(const std::shared_ptr<Context>&)> Handler;

public:
	Transport(const Transport&) = delete;
	void operator=(Transport const&) = delete;
	Transport(Transport&& tmp) = delete;
	Transport& operator=(Transport&& tmp) = delete;

	Transport();
	virtual ~Transport() = default;

	virtual bool processing(const std::shared_ptr<Connection>& connection) = 0;
};
