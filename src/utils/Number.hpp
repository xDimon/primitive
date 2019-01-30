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
		T srcValue,
		size_t numberOfDigitForBegginerRound = 10,
		size_t numberOfAllSagnifficantDigits = 10,
		size_t factor = 1,
		int lastDigitForPsychoRound = -1
	)
	{
		typename std::enable_if<std::is_floating_point<T>::value, bool>::type detect();

		if (srcValue < 0.01)
		{
			return std::ceil(srcValue * 100) / 100;
		}
		else if (srcValue < 0.10)
		{
			return std::round(srcValue * 100) / 100;
		}
		else if (srcValue < 1.00)
		{
			return std::floor(srcValue * 100) / 100;
		}

		// Формируем шаблон для сравнения
		T threshold = std::pow(10, numberOfDigitForBegginerRound);

		// Вычисляем множитель для округления до значащих цифр
		T m1 = 1;

		for (;;)
		{
			if (srcValue * m1 >= threshold) break;
			m1 *= 10;
		}
		for (;;)
		{
			if (srcValue * m1 <= threshold) break;
			m1 /= 10;
		}

		// Вычисляем множитель для значащих цифр после округления
		T m2 = m1 * std::pow(10, numberOfAllSagnifficantDigits - numberOfDigitForBegginerRound);

		// Выравниваем последний разряд
		T t1 = std::round(srcValue * m2 + 1) / m2;

		// Нормируем кратно в последнем значащем разряде
		T t2 = std::floor(t1 * m1 / factor) * factor / m1;

		// Психологическое округление
		T t3 = (lastDigitForPsychoRound > 0 && lastDigitForPsychoRound <= 9) ? ((t2 * m2 - (10 - lastDigitForPsychoRound)) / m2) : t2;

		// Отбрасываем дробную часть копеек
		T t4 = std::floor(std::round(t3 * 1000) / 1000 * 100) / 100;

		return t4;
	}

};
