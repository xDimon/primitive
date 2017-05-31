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
#include "../utils/Shareable.hpp"
#include "../net/AcceptorFactory.hpp"
#include "../configs/Setting.hpp"
#include "../serialization/SerializerFactory.hpp"
#include "../utils/Named.hpp"
#include "../utils/Context.hpp"

class Transport : public Shareable<Transport>, public Named
{
private:
	std::shared_ptr<AcceptorFactory::Creator> _acceptorCreator;
	std::weak_ptr<Connection> _acceptor;

protected:
	Log _log;

public:

	Transport(const Setting& setting);
	virtual ~Transport();

	bool enable();
	bool disable();

	virtual bool processing(std::shared_ptr<Connection> connection) = 0;
};
