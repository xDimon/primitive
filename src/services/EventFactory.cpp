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
// File created on: 2017.06.15

// EventFactory.cpp


#include "EventFactory.hpp"

bool EventFactory::reg(const std::string& name, std::shared_ptr<Event>(* creator)(const std::shared_ptr<Context>&, const SVal*, ServerTransport::Transmitter))
{
	auto& factory = getInstance();

	auto i = factory._creators.find(name);
	if (i != factory._creators.end())
	{
		throw std::runtime_error(std::string("Attepmt to register event with the same name (") + name + ")");
	}
	factory._creators.emplace(name, creator);
	return true;
}

std::shared_ptr<Event> EventFactory::create(const std::string& name, const std::shared_ptr<Context>& context, const SVal* input, ServerTransport::Transmitter transmitter)
{
	auto& factory = getInstance();

	auto i = factory._creators.find(name);
	if (i == factory._creators.end())
	{
		return nullptr;
	}
	return i->second(context, input, transmitter);
}
