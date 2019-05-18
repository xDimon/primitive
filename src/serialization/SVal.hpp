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
// File created on: 2017.04.08

// SVal.hpp


#pragma once

#include <vector>
#include <map>
#include <string>
#include "SNull.hpp"
#include "SBool.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SStr.hpp"
#include "SBinary.hpp"

class SArr;
class SObj;

class SVal final
{
public:
	enum class Type
	{
		Undefined,
		Null,
		Bool,
		Integer,
		Float,
		String,
		Binary,
		Array,
		Object
	};

private:
	union Storage
	{
		SNull vNull;
		SBool vBool;
		SInt vInt;
		SFloat vFloat;
		SBinary vBin;
		std::string vStr;
		std::vector<SVal> vArr;
		std::map<std::string, SVal> vObj;

		Storage() {};
		~Storage() {};
	} _storage;

	Type _type;

public:
	SVal()
	: _type(Type::Undefined)
	{}

	template<typename T, typename std::enable_if<std::is_base_of<SBase, T>::value, void>::type* = nullptr>
	SVal(T&& value) noexcept
	{
		new (&_storage) T(value);
		_type = type(value);
	}

	template<typename T, typename std::enable_if<std::is_base_of<SBase, T>::value, void>::type* = nullptr>
	SVal(const T& value)
	{
		new (&_storage) T(value);
		_type = type(value);
	}

	explicit SVal(nullptr_t)
	{
		new (&_storage) SNull();
		_type = Type::Null;
	}

	explicit SVal(bool value)
	{
		new (&_storage) SBool(value);
		_type = Type::Bool;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value, void>::type* = nullptr>
	SVal(T value)
	{
		typename std::enable_if<std::is_integral<T>::value, bool>::type detect();
		new (&_storage) SInt(value);
		_type = Type::Integer;
	}

	template<typename T, typename std::enable_if<std::is_floating_point<T>::value, void>::type* = nullptr>
	SVal(T value)
	{
		typename std::enable_if<std::is_floating_point<T>::value, bool>::type detect();
		new (&_storage) SFloat(value);
		_type = Type::Float;
	}

	explicit SVal(const char* value)
	{
		new (&_storage) std::string(value);
		_type = Type::String;
	}

	SVal(std::string&& value)
	{
		new (&_storage) SStr(std::move(value));
		_type = Type::String;
	}

	SVal(const std::string& value)
	{
		new (&_storage) SStr(value);
		_type = Type::String;
	}

	SVal(const void* data, size_t size)
	{
		new (&_storage) SBinary(data, size);
		_type = Type::Binary;
	}

	~SVal()
	{
		clear();
	}

	// Copy-constructor
	SVal(const SVal& that)
	{
		switch (that._type)
		{
			case Type::Null:
				new (&_storage) SNull(that._storage.vNull);
				break;
			case Type::Bool:
				new (&_storage) SBool(that._storage.vBool);
				break;
			case Type::Integer:
				new (&_storage) SInt(that._storage.vInt);
				break;
			case Type::Float:
				new (&_storage) SFloat(that._storage.vFloat);
				break;
			case Type::Binary:
				new (&_storage) SBinary(that._storage.vBin);
				break;
			case Type::String:
				new (&_storage) std::string(that._storage.vStr);
				break;
			case Type::Array:
				new (&_storage) std::vector<SVal>(that._storage.vArr);
				break;
			case Type::Object:
				new (&_storage) std::map<std::string, SVal>(that._storage.vObj);
				break;
			default:
				break;
		}
		_type = that._type;
	}

	// Move-constructor
	SVal(SVal&& that) noexcept
	{
		switch (that._type)
		{
			case Type::Null:
				new (&_storage) SNull(that._storage.vNull);
				break;
			case Type::Bool:
				new (&_storage) SBool(that._storage.vBool);
				break;
			case Type::Integer:
				new (&_storage) SInt(that._storage.vInt);
				break;
			case Type::Float:
				new (&_storage) SFloat(that._storage.vFloat);
				break;
			case Type::Binary:
				new (&_storage) SBinary(std::move(that._storage.vBin));
				break;
			case Type::String:
				new (&_storage) std::string(std::move(that._storage.vStr));
				break;
			case Type::Array:
				new (&_storage) std::vector<SVal>(std::move(that._storage.vArr));
				break;
			case Type::Object:
				new (&_storage) std::map<std::string, SVal>(std::move(that._storage.vObj));
				break;
			default:
				break;
		}
		_type = that._type;
		that.clear();
	}

	// Copy-assignment
	virtual SVal& operator=(SVal const& that)
	{
		clear();
		switch (that._type)
		{
			case Type::Null:
				new (&_storage) SNull(that._storage.vNull);
				break;
			case Type::Bool:
				new (&_storage) SBool(that._storage.vBool);
				break;
			case Type::Integer:
				new (&_storage) SInt(that._storage.vInt);
				break;
			case Type::Float:
				new (&_storage) SFloat(that._storage.vFloat);
				break;
			case Type::Binary:
				new (&_storage) SBinary(that._storage.vBin);
				break;
			case Type::String:
				new (&_storage) std::string(that._storage.vStr);
				break;
			case Type::Array:
				new (&_storage) std::vector<SVal>(that._storage.vArr);
				break;
			case Type::Object:
				new (&_storage) std::map<std::string, SVal>(that._storage.vObj);
				break;
			default:
				break;
		}
		_type = that._type;
		return *this;
	}

	// Move-assignment
	SVal& operator=(SVal&& that) noexcept
	{
		clear();
		switch (that._type)
		{
			case Type::Null:
				new (&_storage) SNull(that._storage.vNull);
				break;
			case Type::Bool:
				new (&_storage) SBool(that._storage.vBool);
				break;
			case Type::Integer:
				new (&_storage) SInt(that._storage.vInt);
				break;
			case Type::Float:
				new (&_storage) SFloat(that._storage.vFloat);
				break;
			case Type::Binary:
				new (&_storage) SBinary(std::move(that._storage.vBin));
				break;
			case Type::String:
				new (&_storage) std::string(std::move(that._storage.vStr));
				break;
			case Type::Array:
				new (&_storage) std::vector<SVal>(std::move(that._storage.vArr));
				break;
			case Type::Object:
				new (&_storage) std::map<std::string, SVal>(std::move(that._storage.vObj));
				break;
			default:
				break;
		}
		_type = that._type;
		that.clear();
		return *this;
	}

	template<typename T, typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value, void>::type* = nullptr>
	operator T() const
	{
		switch (_type)
		{
			case Type::Null:
				return _storage.vNull;
			case Type::Bool:
				return _storage.vBool;
			case Type::Integer:
				return _storage.vInt;
			case Type::Float:
				return _storage.vFloat;
			case Type::Binary:
				return _storage.vBin;
			case Type::String:
				return static_cast<const SStr&>(_storage.vStr);
			case Type::Array:
				return _storage.vArr.size();
			case Type::Object:
				return _storage.vObj.size();
			default:
				return T();
		}
	}

	operator std::string() const
	{
		switch (_type)
		{
			case Type::Null:
				return (std::string)_storage.vNull;
			case Type::Bool:
				return (std::string)_storage.vBool;
			case Type::Integer:
				return (std::string)_storage.vInt;
			case Type::Float:
				return (std::string)_storage.vFloat;
			case Type::Binary:
				return (std::string)_storage.vBin;
			case Type::String:
				return _storage.vStr;
			case Type::Array:
			{
				std::ostringstream oss;
				oss << "[array#" << this << '(' << _storage.vArr.size() << ")]";
				return oss.str();
			}
			case Type::Object:
			{
				std::ostringstream oss;
				oss << "[object#" << this << '(' << _storage.vObj.size() << ")]";
				return oss.str();
			}
			default:
				return std::string();
		}
	}

	void clear()
	{
		switch (_type)
		{
			case Type::Null:
				_storage.vNull.~SNull();
				break;
			case Type::Bool:
				_storage.vBool.~SBool();
				break;
			case Type::Integer:
				_storage.vInt.~SInt();
				break;
			case Type::Float:
				_storage.vFloat.~SFloat();
				break;
			case Type::Binary:
				_storage.vBin.~SBinary();
				break;
			case Type::String:
				_storage.vStr.std::string::~basic_string();
				break;
			case Type::Array:
				_storage.vArr.std::vector<SVal>::~vector();
				break;
			case Type::Object:
				_storage.vObj.std::map<std::string, SVal>::~map();
				break;
			default:
				break;
		}
		_type = Type::Undefined;
	}

	bool isUndefined() const
	{
		return _type == Type::Undefined;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SNull, T>::value, bool>
	is() const
	{
		return _type == Type::Null;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SBool, T>::value, bool>
	is() const
	{
		return _type == Type::Bool;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SInt, T>::value, bool>
	is() const
	{
		return _type == Type::Integer;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SFloat, T>::value, bool>
	is() const
	{
		return _type == Type::Float;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SStr, T>::value, bool>
	is() const
	{
		return _type == Type::String;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SBinary, T>::value, bool>
	is() const
	{
		return _type == Type::Binary;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SArr, T>::value, bool>
	is() const
	{
		return _type == Type::Array;
	}

	template<typename T>
	typename std::enable_if_t<std::is_same<SObj, T>::value, bool>
	is() const
	{
		return _type == Type::Object;
	}

 	template<typename T, typename std::enable_if<std::is_base_of<SBase, T>::value, bool>::type* = nullptr>
	const T& as() const
	{
		if (type(*(T*)nullptr) == _type)
		{
			return *reinterpret_cast<const T*>(&_storage);
		}
		throw std::runtime_error("Value has other type");
	}

	template<typename T, typename std::enable_if<
		!std::is_base_of<SBase, T>::value &&
		!std::is_same<std::string, T>::value
//		!std::is_same<std::vector<SVal>, T>::value &&
//		!std::is_same<std::map<std::string, SVal> , T>::value,
	,void>::type* = nullptr>
	const T& as() const
	{
		return operator T();
	}

	template<typename T, typename std::enable_if<std::is_same<std::string, T>::value, bool>::type* = nullptr>
	const T& as() const
	{
		if (_type == Type::String)
		{
			return _storage.vStr;
		}
		throw std::runtime_error("Value has other type");
	}

	template<typename T, typename std::enable_if<std::is_base_of<SBase, T>::value, bool>::type* = nullptr>
	T& as()
	{
		if (type(*(T*)nullptr) == _type)
		{
			return *reinterpret_cast<T*>(&_storage);
		}
		throw std::runtime_error("Value has other type");
	}

	template<typename T, typename std::enable_if<
		!std::is_base_of<SBase, T>::value &&
			!std::is_same<std::string, T>::value
//		!std::is_same<std::vector<SVal>, T>::value &&
//		!std::is_same<std::map<std::string, SVal> , T>::value,
		,void>::type* = nullptr>
	T& as()
	{
		return operator T();
	}

	template<typename T, typename std::enable_if<std::is_same<std::string, T>::value, bool>::type* = nullptr>
	T& as()
	{
		if (_type == Type::String)
		{
			return _storage.vStr;
		}
		throw std::runtime_error("Value has other type");
	}

	Type type() const
	{
		return _type;
	}

private:
	template<typename T>
	static typename std::enable_if_t<std::is_same<SVal, T>::value, Type>
	type(const T& value)
	{
		return value._type;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SNull, T>::value, Type>
	type(const T& value)
	{
		return Type::Null;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SBool, T>::value, Type>
	type(const T& value)
	{
		return Type::Bool;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SInt, T>::value, Type>
	type(const T& value)
	{
		return Type::Integer;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SFloat, T>::value, Type>
	type(const T& value)
	{
		return Type::Float;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SStr, T>::value, Type>
	type(const T& value)
	{
		return Type::String;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SBinary, T>::value, Type>
	type(const T& value)
	{
		return Type::Binary;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SArr, T>::value, Type>
	type(const T& value)
	{
		return Type::Array;
	}

	template<typename T>
	static typename std::enable_if_t<std::is_same<SObj, T>::value, Type>
	type(const T& value)
	{
		return Type::Object;
	}
};
