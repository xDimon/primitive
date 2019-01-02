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
// File created on: 2017.03.09

// ConnectionEvent.hpp


#pragma once

struct ConnectionEvent
{
	enum class Type : uint32_t {
		READ	= 1<<0,
		WRITE	= 1<<1,
		HUP		= 1<<2,
		HALFHUP	= 1<<3,
		TIMEOUT	= 1<<6,
		ERROR	= 1<<7
	};

	static std::string code(uint32_t events)
	{
		std::string result;
		if (events & static_cast<uint32_t>(Type::READ))		result.push_back('R');
		if (events & static_cast<uint32_t>(Type::WRITE))	result.push_back('W');
		if (events & static_cast<uint32_t>(Type::HUP))		result.push_back('C');
		if (events & static_cast<uint32_t>(Type::HALFHUP))	result.push_back('H');
		if (events & static_cast<uint32_t>(Type::TIMEOUT))	result.push_back('T');
		if (events & static_cast<uint32_t>(Type::ERROR))	result.push_back('E');
		return result;
	}
};
