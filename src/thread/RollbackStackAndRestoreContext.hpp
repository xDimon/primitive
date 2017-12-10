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
// File created on: 2017.09.27

// RollbackStackAndRestoreContext.hpp


#pragma once


#include <exception>
#include "Task.hpp"

class RollbackStackAndRestoreContext: public std::exception
{
private:
	std::shared_ptr<Task> _task;

public:
	RollbackStackAndRestoreContext(const std::shared_ptr<Task>& task) noexcept
	: _task(task)
	{};
	~RollbackStackAndRestoreContext() noexcept override
	{
		auto& task = *_task;
		_task.reset();

		task.restoreContext();
		task.restoreCtx();
	};

	// Non-copyable
	RollbackStackAndRestoreContext(const RollbackStackAndRestoreContext&) = delete; // Copy-constructor
	void operator=(RollbackStackAndRestoreContext const&) = delete; // Copy-assignment

	// Non-moveable
	RollbackStackAndRestoreContext(RollbackStackAndRestoreContext&&) = default; // Move-constructor
	RollbackStackAndRestoreContext& operator=(RollbackStackAndRestoreContext&&) = delete; // Move-assignment

	const char* what() const noexcept override
	{
		return "Rollback current stack and restore parent";
	}
};
