// Copyright © 2018 Dmitriy Khaustov
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
// File created on: 2018.01.13

// DoAtExitScope.hpp


#pragma once

#include <functional>

class DoAtExitScope final
{
private:
	std::function<void()> _func;

public:
	DoAtExitScope(std::function<void()>&& func)
	: _func(std::move(func))
	{
	}

	DoAtExitScope() = delete; // Copy-constructor
	DoAtExitScope(const DoAtExitScope&) = delete; // Copy-constructor
	DoAtExitScope& operator=(DoAtExitScope const&) = delete; // Copy-assignment
	DoAtExitScope(DoAtExitScope&&) noexcept = delete; // Move-constructor
	DoAtExitScope& operator=(DoAtExitScope&&) noexcept = delete; // Move-assignment

	~DoAtExitScope()
	{
		_func();
	}
};
