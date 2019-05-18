// Copyright © 2019 Dmitriy Khaustov
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
// File created on: 2019.05.14

// Amf3Exeption.hpp


#pragma once

#include <exception>
#include <string>
#include <sstream>

class Amf3Exeption final: public std::exception
{
	std::string _msg;

public:
	Amf3Exeption() = delete; // Default-constructor
	Amf3Exeption(Amf3Exeption&&) noexcept = default; // Move-constructor
	Amf3Exeption(const Amf3Exeption&) = delete; // Copy-constructor
	~Amf3Exeption() override = default; // Destructor
	Amf3Exeption& operator=(Amf3Exeption&&) noexcept = delete; // Move-assignment
	Amf3Exeption& operator=(const Amf3Exeption&) = delete; // Copy-assignment

	Amf3Exeption(std::string msg, size_t pos, std::istream& is);

	Amf3Exeption(std::string msg, size_t pos);

	Amf3Exeption(std::string msg);

	const char* what() const noexcept override
	{
		return _msg.c_str();
	}
};
