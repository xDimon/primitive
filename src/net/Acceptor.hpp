// Copyright © 2017-2019 Dmitriy Khaustov
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
// File created on: 2017.04.28

// Acceptor.hpp


#pragma once


#include "Connection.hpp"
#include "../configs/Setting.hpp"
#include "../utils/Named.hpp"
#include "../transport/ServerTransport.hpp"

class Acceptor : public Connection
{
public:
	Acceptor() = delete;
	Acceptor(const Acceptor&) = delete;
	Acceptor& operator=(const Acceptor&) = delete;
	Acceptor(Acceptor&& tmp) noexcept = delete;
	Acceptor& operator=(Acceptor&& tmp) noexcept = delete;

	explicit Acceptor(const std::shared_ptr<ServerTransport>& transport);
	~Acceptor() override = default;
};
