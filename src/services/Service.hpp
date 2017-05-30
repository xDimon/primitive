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
// File created on: 2017.05.30

// Service.hpp


#pragma once


#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"
#include "../serialization/SerializerFactory.hpp"

class Service : public Shareable<Service>, public Named
{
private:
	std::shared_ptr<SerializerFactory::Creator> _serializerCreator;

public:
	Service(const Setting& setting);
	virtual ~Service();

	virtual SVal* processing(SVal* input) = 0;

	virtual std::shared_ptr<Serializer> serializer() const
	{
		return (*_serializerCreator)();
	}
};
