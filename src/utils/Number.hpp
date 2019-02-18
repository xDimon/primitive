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
// File created on: 2017.07.06

// Number.hpp


#pragma once

#include <climits>
#include <cmath>
#include <iostream>
#include <iomanip>

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

	template<typename T>
	double smartRound(
		T value,
		size_t numberOfDigitForBegginerRound = 10,
		size_t numberOfAllSagnifficantDigits = 10,
		size_t factor = 1,
		int lastDigitForPsychoRound = -1
	)
	{
		typename std::enable_if<std::is_floating_point<T>::value, bool>::type detect();

		if (value < 0.01)
		{
			return std::ceil(value * 100) / 100;
		}
		else if (value < 0.10)
		{
			return std::round(value * 100) / 100;
		}

		// Уменьшаем на единицу незначащего разряда ()
		value -= std::pow(10, -static_cast<intmax_t>(numberOfAllSagnifficantDigits+1));

		// Вычисляем множитель для округления до значащих цифр
		auto e1 = static_cast<intmax_t>(std::floor(numberOfDigitForBegginerRound - std::log10(value)));

		// Вычисляем множитель для значащих цифр после округления
		auto e2 = static_cast<intmax_t>(numberOfAllSagnifficantDigits - numberOfDigitForBegginerRound);

		// Нормируем делитель, для кратности
		auto f = (e1 - e2 <= 0) ? (factor * std::pow(10, e2)) : std::pow(10, e2 + 1);

		// Кратное округление
		auto factorRounded = static_cast<uintmax_t>(std::floor(std::round(value * std::pow(10, e1 + e2)) / f) * f);

		// Психологическое округление
		auto psychoRounded = static_cast<uintmax_t>(
			(lastDigitForPsychoRound > 0 && lastDigitForPsychoRound <= 9 && (e1 - e2) <= 1) ?
			(factorRounded - 1) :
			factorRounded
		);

		auto moduloRounded = std::floor(psychoRounded / std::pow(10, e1 + e2 - 2)) / 100;

		return moduloRounded;
	}
};
