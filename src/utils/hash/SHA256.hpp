/*
 * Updated to C++, zedwood.com 2012
 * Based on Olivier Gay's version
 * See Modified BSD License below:
 *
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Issue date:  04/30/2005
 * http://www.ouah.org/ogay/sha2/
 *
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//  Adapt for project
//      Dmitriy Khaustov <khaustov.dm@gmail.com>
//
// File created on: 2017.02.25

// SHA256.hpp


#pragma once


#include <string>
#include <array>

class SHA256
{
protected:
	const static uint32_t sha256_k[];
	static const uint32_t SHA224_256_BLOCK_SIZE = (512 / 8);

public:
	void init();

	void update(const uint8_t* message, size_t len);

	void final(uint8_t* digest);

	static const uint32_t DIGEST_SIZE = (256 / 8);

	static std::array<uint8_t, SHA256::DIGEST_SIZE> encode_bin(const void* data, size_t size);
	static std::string encode(const void* data, size_t size);
	static std::string encode(const std::string& str)
	{
		return encode(str.data(), str.size());
	}

protected:
	void transform(const uint8_t* message, size_t block_nb);

	size_t m_tot_len;
	size_t m_len;
	uint8_t m_block[2 * SHA224_256_BLOCK_SIZE];
	uint32_t m_h[8];
};


#define SHA2_SHFR(x, n)    (x >> n)
#define SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define SHA256_F1(x) (SHA2_ROTR(x,  2) ^ SHA2_ROTR(x, 13) ^ SHA2_ROTR(x, 22))
#define SHA256_F2(x) (SHA2_ROTR(x,  6) ^ SHA2_ROTR(x, 11) ^ SHA2_ROTR(x, 25))
#define SHA256_F3(x) (SHA2_ROTR(x,  7) ^ SHA2_ROTR(x, 18) ^ SHA2_SHFR(x,  3))
#define SHA256_F4(x) (SHA2_ROTR(x, 17) ^ SHA2_ROTR(x, 19) ^ SHA2_SHFR(x, 10))
#define SHA2_UNPACK32(x, str)                 \
{                                             \
    *((str) + 3) = (uint8_t) ((x)      );       \
    *((str) + 2) = (uint8_t) ((x) >>  8);       \
    *((str) + 1) = (uint8_t) ((x) >> 16);       \
    *((str) + 0) = (uint8_t) ((x) >> 24);       \
}
#define SHA2_PACK32(str, x)                   \
{                                             \
    *(x) =   ((uint32_t) *((str) + 3)      )    \
           | ((uint32_t) *((str) + 2) <<  8)    \
           | ((uint32_t) *((str) + 1) << 16)    \
           | ((uint32_t) *((str) + 0) << 24);   \
}
