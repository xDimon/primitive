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
// File created on: 2017.06.15

// SipHash.cpp


#include <cstring>
#include <algorithm>
#include "SipHash.hpp"

inline
static uint64_t rotl(uint64_t x, uint64_t b)
// Return the bits of the specified 'x' rotated to the left by the
// specified 'b' number of bits.  Bits that are rotated off the end are
// wrapped around to the beginning.
{
	return (x << b) | (x >> (64 - b));
}

inline
static void sipround(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3)
// Mix the specified 'v0', 'v1', 'v2', and 'v3' in a "Sip Round" as
// described in the SipHash algorithm
{
	v0 += v1;
	v1 = rotl(v1, 13);
	v1 ^= v0;
	v0 = rotl(v0, 32);
	v2 += v3;
	v3 = rotl(v3, 16);
	v3 ^= v2;
	v0 += v3;
	v3 = rotl(v3, 21);
	v3 ^= v0;
	v2 += v1;
	v1 = rotl(v1, 17);
	v1 ^= v2;
	v2 = rotl(v2, 32);
}

inline
static uint64_t u8to64_le(const uint8_t* p)
// Return the 64-bit integer representation of the specified 'p' taking
// into account endianness.  Undefined unless 'p' points to at least eight
// bytes of initialized memory.
{
	uint64_t ret;
	memcpy(&ret, p, sizeof(ret));
	return le64toh(ret);
}


SipHash::SipHash(const char (& seed)[16]) noexcept
	: d_v0(0x736f6d6570736575ULL)
	, d_v1(0x646f72616e646f6dULL)
	, d_v2(0x6c7967656e657261ULL)
	, d_v3(0x7465646279746573ULL)
	, d_bufSize(0)
	, d_totalLength(0)
{
	uint64_t k0 = u8to64_le(reinterpret_cast<const uint8_t*>(&seed[0]));
	uint64_t k1 = u8to64_le(reinterpret_cast<const uint8_t*>(&seed[8]));

	d_v0 ^= k0;
	d_v1 ^= k1;
	d_v2 ^= k0;
	d_v3 ^= k1;
}

void
SipHash::operator()(const void* data, size_t numBytes) noexcept
{
	uint8_t const* in = static_cast<const uint8_t*>(data);

	d_totalLength += numBytes;
	if (d_bufSize + numBytes < 8)
	{
		std::copy(in, in + numBytes, d_buf + d_bufSize);
		d_bufSize += numBytes;
		return;                                                       // RETURN
	}
	if (d_bufSize > 0)
	{
		size_t t = 8 - d_bufSize;
		std::copy(in, in + t, d_buf + d_bufSize);
		uint64_t m = u8to64_le(d_buf);
		d_v3 ^= m;
		sipround(d_v0, d_v1, d_v2, d_v3);
		sipround(d_v0, d_v1, d_v2, d_v3);
		d_v0 ^= m;
		in += t;
		numBytes -= t;
	}
	d_bufSize = numBytes & 7;
	uint8_t const* const end = in + (numBytes - d_bufSize);
	for (; in != end; in += 8)
	{
		uint64_t m = u8to64_le(in);
		d_v3 ^= m;
		sipround(d_v0, d_v1, d_v2, d_v3);
		sipround(d_v0, d_v1, d_v2, d_v3);
		d_v0 ^= m;
	}
	std::copy(end, end + d_bufSize, d_buf);
}

SipHash::result_type SipHash::computeHash() noexcept
{
	// The "FALL THROUGH" comments here are necessary to avoid the
	// implicit-fallthrough warnings that GCC 7 introduces.  We could
	// instead use GNU C's __attribute__(fallthrough) vendor
	// extension or C++17's [[fallthrough]] attribute but these would
	// need to be hidden from the Oracle and IBM compilers.

	result_type b = static_cast<uint64_t>(d_totalLength) << 56;
	switch (d_bufSize)
	{
		case 7: b |= static_cast<uint64_t>(d_buf[6]) << 48;  // FALL THROUGH
		case 6: b |= static_cast<uint64_t>(d_buf[5]) << 40;  // FALL THROUGH
		case 5: b |= static_cast<uint64_t>(d_buf[4]) << 32;  // FALL THROUGH
		case 4: b |= static_cast<uint64_t>(d_buf[3]) << 24;  // FALL THROUGH
		case 3: b |= static_cast<uint64_t>(d_buf[2]) << 16;  // FALL THROUGH
		case 2: b |= static_cast<uint64_t>(d_buf[1]) << 8;   // FALL THROUGH
		case 1: b |= static_cast<uint64_t>(d_buf[0]);        // FALL THROUGH
		case 0: break;
		default: break;
	}
	d_v3 ^= b;
	sipround(d_v0, d_v1, d_v2, d_v3);
	sipround(d_v0, d_v1, d_v2, d_v3);
	d_v0 ^= b;
	d_v2 ^= 0xff;
	sipround(d_v0, d_v1, d_v2, d_v3);
	sipround(d_v0, d_v1, d_v2, d_v3);
	sipround(d_v0, d_v1, d_v2, d_v3);
	sipround(d_v0, d_v1, d_v2, d_v3);
	b = d_v0 ^ d_v1 ^ d_v2 ^ d_v3;
	return b;
}
