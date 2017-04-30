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
// File created on: 2017.04.06

// SslHelper.hpp


#pragma once

#include <openssl/ossl_typ.h>
#include <memory>

class SslHelper
{
private:
	SslHelper();
	virtual ~SslHelper();

	SslHelper(SslHelper const&) = delete;
	void operator= (SslHelper const&) = delete;

	static SslHelper &getInstance()
	{
		static SslHelper instance;
		return instance;
	}

	std::shared_ptr<SSL_CTX> _context;

public:
	static std::shared_ptr<SSL_CTX> context()
	{
		return getInstance()._context;
	}
};