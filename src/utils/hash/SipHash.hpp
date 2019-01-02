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

// SipHash.hpp


#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

class SipHash
{
private:
	// DATA
	uint64_t d_v0;
	uint64_t d_v1;
	uint64_t d_v2;
	uint64_t d_v3;

	// Stores the intermediate state of the algorithm as values areaccumulated
	union
	{
		uint64_t d_alignment; // Provides alignment
		unsigned char d_buf[8];	// Used to buffer data until we have enough to do a full round of computation as specified by the algorithm.
	};

	size_t d_bufSize; // The length of the data currently stored in the buffer.

	size_t d_totalLength; // The total length of all data that has been passed into the algorithm.

public:
	// TYPES
	typedef uint64_t result_type; // Typedef indicating the value type returned by this algorithm.

	SipHash(const SipHash& original) = delete; // Do not allow copy construction.
	SipHash& operator=(const SipHash& rhs) = delete; // Do not allow assignment.

	// CREATORS
	explicit SipHash(const char (&seed)[16]) noexcept;
	// Create a 'SipHash', seeded with a 128-bit
	// ('k_SEED_LENGTH' bytes) seed pointed to by the specified 'seed'.
	// Each bit of the supplied seed will contribute to the final hash
	// produced by 'computeHash()'.  The behaviour is undefined unless
	// 'seed' points to at least 16 bytes of initialized memory.  Note that
	// if data in 'seed' is not random, all guarantees of security and
	// Denial of Service (DoS) protection are void.

	~SipHash() = default;

	// MANIPULATORS
	void operator()(const void* data, size_t numBytes) noexcept;
	void operator()(const std::string& str) noexcept
	{
		operator()(str.data(), str.length());
	}
	// Incorporate the specified 'data', of at least the specified
	// 'numBytes', into the internal state of the hashing algorithm.  Every
	// bit of data incorporated into the internal state of the algorithm
	// will contribute to the final hash produced by 'computeHash()'.  The
	// same hash will be produced regardless of whether a sequence of bytes
	// is passed in all at once or through multiple calls to this member
	// function.  Input where 'numBytes' is 0 will have no effect on the
	// internal state of the algorithm.  The behaviour is undefined unless
	// 'data' points to a valid memory location with at least 'numBytes'
	// bytes of initialized memory.

	result_type computeHash() noexcept;
	// Return the finalized version of the hash that has been accumulated.
	// Note that this changes the internal state of the object, so calling
	// 'computeHash()' multiple times in a row will return different
	// results, and only the first result returned will match the expected
	// result of the algorithm.  Also note that a value will be returned,
	// even if data has not been passed into 'operator()'
};
