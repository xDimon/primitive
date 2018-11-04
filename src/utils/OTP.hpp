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
// File created on: 2018.10.31

// OTP.hpp


#pragma once

#include <string>

class OTP final
{
private:
	std::string _key;

public:
	OTP() = delete; // Default-constructor
	OTP(OTP&&) noexcept = delete; // Move-constructor
	OTP(const OTP&) = delete; // Copy-constructor
	~OTP() = default; // Destructor
	OTP& operator=(OTP&&) noexcept = delete; // Move-assignment
	OTP& operator=(OTP const&) = delete; // Copy-assignment

	OTP(std::string key)
	: _key(std::move(key))
	{
	}

	std::string getHOTP(uint64_t count);
	std::string getTOTP();

	static std::string generateSecret();
};
