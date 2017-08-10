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
// File created on: 2017.06.05

// ClientTransport.hpp


#pragma once


#include "../utils/Shareable.hpp"
#include "Transport.hpp"

class ClientTransport : public Shareable<ClientTransport>, public Transport
{
public:
	ClientTransport(const ClientTransport&) = delete;
	void operator=(ClientTransport const&) = delete;
	ClientTransport(ClientTransport&&) = delete;
	ClientTransport& operator=(ClientTransport&&) = delete;

	ClientTransport();
	~ClientTransport() override = default;
};
