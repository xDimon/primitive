// Copyright © 2018-2019 Dmitriy Khaustov
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
// File created on: 2018.07.07

// Compressor.hpp


#pragma once

#include <cstdint>
#include <vector>

class Compressor
{
//public:
//	static const uint32_t STRICT = 1u<<0;

protected:
	uint32_t _flags;

public:
	Compressor() = delete;
	Compressor(const Compressor&) = delete;
	Compressor& operator=(Compressor const&) = delete;
	Compressor(Compressor&&) noexcept = delete;
	Compressor& operator=(Compressor&&) noexcept = delete;

	explicit Compressor(uint32_t flags)
	: _flags(flags)
	{}
	virtual ~Compressor() = default;

	virtual void deflate(const std::vector<char>& in, std::vector<char>& out) = 0;
	virtual void inflate(const std::vector<char>& in, std::vector<char>& out) = 0;
};

#include "CompressorFactory.hpp"

#define REGISTER_COMPRESSOR(Type,Class) const Dummy Class::__dummy = \
    CompressorFactory::reg(                                                                     \
        #Type,                                                                                  \
        [](uint32_t flags){                                                                     \
            return std::shared_ptr<Compressor>(new Class(flags));                               \
        }                                                                                       \
    );

#define DECLARE_COMPRESSOR(Class) \
public:                                                                                         \
	Class() = delete;                                                                           \
	Class(const Class&) = delete;                                                               \
    Class& operator=(Class const&) = delete;                                                    \
	Class(Class&&) noexcept = delete;                                                           \
	Class& operator=(Class&&) noexcept = delete;                                                \
                                                                                                \
private:                                                                                        \
    explicit Class(uint32_t flags): Compressor(flags) {}                                        \
                                                                                                \
public:                                                                                         \
    ~Class() override = default;                                                                \
                                                                                                \
	void deflate(const std::vector<char>& in, std::vector<char>& out) override;                 \
	void inflate(const std::vector<char>& in, std::vector<char>& out) override;                 \
                                                                                                \
private:                                                                                        \
    static const Dummy __dummy;
