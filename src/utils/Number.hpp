// Copyright © 2017-2018 Dmitriy Khaustov
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
// File created on: 2017.07.06

// Number.hpp


#pragma once

#include <climits>


namespace Number
{
	template<typename D, typename S>
	void assign(D& dst, const S& src, const D& def = D())
	{
		if (
			src > std::numeric_limits<D>::max() ||
			src < std::numeric_limits<D>::min()
		)
		{
			dst = def;
		}
		else
		{
			dst = static_cast<D>(src);
		}
	}

	template<typename T>
	T obfuscate(T src, size_t rounds = 0)
	{
		size_t bits = sizeof(T) * CHAR_BIT;

		if (rounds == 0)
		{
			if (bits > 32) rounds = 4;
			else if (bits > 16) rounds = 3;
			else if (bits > 8) rounds = 2;
			else rounds = 1;
		}

		T s;
		T d = src;

		for (size_t r = 0; r < rounds; r++)
		{
			s = d;
			d = 0;

			for (size_t i = 0; i < (bits >> 1); i++)
			{
				d |= ((s >> i) & 1) << (bits - 1 - i * 2);
			}
			for (size_t i = (bits >> 1); i < bits; i++)
			{
				d |= ((s >> i) & 1) << ((i - (bits >> 1)) * 2);
			}
		}

		return d;
	}

	template<typename T>
	T deobfuscate(T src, size_t rounds = 0)
	{
		size_t bits = sizeof(T) * CHAR_BIT;

		if (rounds == 0)
		{
			if (bits > 32) rounds = 4;
			else if (bits > 16) rounds = 3;
			else if (bits > 8) rounds = 2;
			else rounds = 1;
		}

		T s;
		T d = src;

		for (size_t r = 0; r < rounds; r++)
		{
			s = d;
			d = 0;

			for (size_t i = 0; i < (bits >> 1); i++)
			{
				d |= ((s >> (bits - 1 - i * 2)) & 1) << i;
			}
			for (size_t i = (bits >> 1); i < bits; i++)
			{
				d |= ((s >> ((i - (bits >> 1)) * 2)) & 1) << i;
			}
		}

		return d;
	}
};
