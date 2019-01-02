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
// File created on: 2017.07.20

// TimeoutWatcher.hpp


#pragma once


#include <chrono>
#include "../utils/Shareable.hpp"

class Timeout;

class TimeoutWatcher final: public Shareable<TimeoutWatcher>
{
private:
	std::weak_ptr<Timeout> _wp;
	size_t _refCounter;

public:
	explicit TimeoutWatcher(const std::shared_ptr<Timeout>&);
	~TimeoutWatcher() override = default;

	void operator()();

	void restart(std::chrono::steady_clock::time_point time);
};
