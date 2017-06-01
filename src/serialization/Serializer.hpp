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
// File created on: 2017.04.30

// Serializer.hpp


#pragma once

#include "SVal.hpp"

class Serializer
{
public:
	virtual SVal* decode(const std::string &data, bool strict = false) = 0;
	virtual std::string encode(const SVal* value) = 0;
};

#include "SerializerFactory.hpp"

#define REGISTER_SERIALIZER(Type,Class) const bool Class::__dummy = \
    SerializerFactory::reg(                                                                     \
        #Type,                                                                                  \
        [](){                                                                                   \
            return std::shared_ptr<Serializer>(new Class());                                    \
        }                                                                                       \
    );

#define DECLARE_SERIALIZER(Class) \
private:                                                                                        \
    Class(): Serializer() {}                                                                    \
    Class(const Class&) = delete;                                                               \
    void operator=(Class const&) = delete;                                                      \
                                                                                                \
public:                                                                                         \
	virtual SVal* decode(const std::string &data, bool strict = false) override;                \
	virtual std::string encode(const SVal* value) override;                           	        \
                                                                                                \
private:                                                                                        \
    static const bool __dummy;
