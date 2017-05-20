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

#include <mutex>
#include "../log/Log.hpp"

class Thread
{
	friend void fake(Thread *thread);
private:
	Log _log;
	std::mutex _mutex;
	std::function<void()> _function;
	std::thread _thread;

	static void run(Thread *);

public:
	Thread(std::function<void()>);
	virtual ~Thread();

	inline void join()
	{
		_thread.join();
	}

	inline auto id() const
	{
		return _thread.get_id();
	}

	void reenter();

	static Thread * self();
};
