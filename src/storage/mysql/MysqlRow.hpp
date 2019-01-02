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
// File created on: 2018.10.08

// MysqlRow.hpp


#pragma once


#include <mysql.h>
#include "../DbRow.hpp"
#include <type_traits>
#include <string>

class MysqlRow final : public DbRow
{
private:
	MYSQL_ROW _data;

public:
	MysqlRow(MysqlRow&&) noexcept = delete; // Move-constructor
	MysqlRow(const MysqlRow&) = delete; // Copy-constructor
	MysqlRow& operator=(MysqlRow&&) noexcept = delete; // Move-assignment
	MysqlRow& operator=(MysqlRow const&) = delete; // Copy-assignment

	MysqlRow() // Default-constructor
	: _data(nullptr)
	{
	}

	~MysqlRow() override = default; // Destructor

	void set(MYSQL_ROW value)
	{
		_data = value;
	}

	MYSQL_ROW get()
	{
		return _data;
	}

	class FieldValue final
	{
	private:
		const char * const _data;

	public:
		FieldValue(const char *const data): _data(data) {}

		bool isNull() const
		{
			return _data == nullptr;
		}

		template<typename T, typename std::enable_if<std::is_integral<T>::value, void>::type* = nullptr>
		operator T() const
		{
			typename std::enable_if<std::is_integral<T>::value, bool>::type detect();
			return static_cast<T>(_data ? std::atoll(_data) : 0);
		}

		template<typename T, typename std::enable_if<std::is_floating_point<T>::value, void>::type* = nullptr>
		operator T() const
		{
			typename std::enable_if<std::is_floating_point<T>::value, bool>::type detect();
			return static_cast<T>(_data ? std::atof(_data) : 0);
		}

		template<typename T, typename std::enable_if<std::is_same<std::string, T>::value, void>::type* = nullptr>
		operator T() const
		{
			typename std::enable_if<std::is_same<std::string, T>::value, void>::type detect();
			return T(_data ? _data : "");
		}
	};

	FieldValue operator[](size_t index) const
	{
		return _data[index];
	}
};
