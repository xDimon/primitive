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
// File created on: 2017.02.28

// Thread.hpp


#pragma once

#include <thread>
#include <event2/event.h>
#include <memory>
#include <evhttp.h>
#include <ucontext.h>

#include "../log/Log.hpp"

class Thread: public Log
{
private:
	std::function<void(ucontext_t*)> _function;
	std::thread _thread;

	ucontext_t _reenterContext;

	static void run(Thread *);

public:
	Thread(std::function<void(ucontext_t*)>);
	virtual ~Thread();

	ucontext_t* reenterContext()
	{
		return &_reenterContext;
	}

	inline void join()
	{
		_thread.join();
	}

	inline auto id()
	{
		return _thread.get_id();
	}

	static Thread * self();
};
