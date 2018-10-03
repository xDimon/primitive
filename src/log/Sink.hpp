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
// File created on: 2017.08.16

// Sink.hpp


#pragma once


#include <P7_Client.h>
#include <P7_Trace.h>
#include "../configs/Setting.hpp"

class Sink final
{
private:
	std::string _name;
	IP7_Client *_p7client;
	IP7_Trace *_p7trace;

public:
	Sink(const Sink&) = delete; // Copy-constructor
	void operator=(Sink const&) = delete; // Copy-assignment
	Sink(Sink&&) = delete; // Move-constructor
	Sink& operator=(Sink&&) = delete; // Move-assignment

	Sink();
	explicit Sink(const Setting& setting);
	~Sink();

	const std::string& name() const
	{
		return _name;
	}

	IP7_Trace& trace()
	{
		return *_p7trace;
	}
};
