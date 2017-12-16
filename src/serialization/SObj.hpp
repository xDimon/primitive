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
// File created on: 2017.04.07

// SObj.hpp


#pragma once

#include <map>
#include <algorithm>
#include <functional>
#include <stddef.h>
#include "SStr.hpp"
#include "SVal.hpp"
#include "SBool.hpp"
#include "SInt.hpp"
#include "SFloat.hpp"
#include "SNull.hpp"
#include "SArr.hpp"

class SObj : public SVal
{
private:
	std::map<const std::string, SVal*> _elements;

public:
	SObj() = default;

	virtual ~SObj()
	{
		for (auto i : _elements)
		{
			delete i.second;
			i.second = nullptr;
		}
		_elements.clear();
	};

	SObj* clone() const override
	{
		SObj *copy = new SObj();
		forEach([&](const std::string& key, const SVal* value){
			copy->insert(key, value->clone());
		});
		return copy;
	}

	SObj(SObj&& tmp)
	{
		_elements.swap(tmp._elements);
	}

	SObj& operator=(SObj&& tmp)
	{
		_elements.swap(tmp._elements);
		return *this;
	}

	void insert(const std::string& key, const SVal* value)
	{
		auto i = _elements.find(key);
		if (i != _elements.end())
		{
			delete i->second;
			i->second = const_cast<SVal*>(value);
		}
		else
		{
			_elements.emplace(key, const_cast<SVal*>(value));
		}
	}

	void insert(const std::string& key, SVal& value)
	{
		insert(key, &value);
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, void>::type
	insert(const std::string& key, T value)
	{
		insert(key, new SInt(value));
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, void>::type
	insert(const std::string& key, SFloat::type value)
	{
		insert(key, new SFloat(value));
	}

	void insert(const std::string& key, double value)
	{
		insert(key, new SFloat(value));
	}

	void insert(const std::string& key, bool value)
	{
		insert(key, new SBool(value));
	}

	void insert(const std::string& key, const char* value)
	{
		insert(key, new SStr(value));
	}

	void insert(const std::string& key, const std::string& value)
	{
		insert(key, new SStr(value));
	}

	void insert(const std::string& key, nullptr_t)
	{
		insert(key, new SNull());
	}

	SVal* get(const std::string& key) const
	{
		auto i = _elements.find(key);
		if (i == _elements.end())
		{
			return nullptr;
		}
		return i->second;
	}

	void forEach(std::function<void(const std::string&, const SVal*)> handler) const
	{
		for (auto const& element : _elements)
		{
			handler(element.first, element.second);
		}
	}

	int64_t getAsInt(const std::string& key, int64_t defaultValue = 0LL) const
	{
		auto val = get(key);
		return val ? (int)(*val) : defaultValue;
	}
	double getAsFlt(const std::string& key, double defaultValue = 0.0) const
	{
		auto val = get(key);
		return val ? (double)(*val) : defaultValue;
	}
	std::string getAsStr(const std::string& key, const std::string& defaultValue = "") const
	{
		auto val = get(key);
		return val ? (std::string)(*val) : defaultValue;
	}
	const SArr* getAsArr(const std::string& key) const
	{
		return dynamic_cast<const SArr*>(get(key));
	}
	const SObj* getAsObj(const std::string& key) const
	{
		return dynamic_cast<const SObj*>(get(key));
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value, void>::type
	lookup(const std::string& key, T &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!dynamic_cast<const SInt *>(element) && strict)
		{
			throw std::runtime_error("Field '" + key + "' isn't integer");
		}
		auto sInt = dynamic_cast<const SInt *>(element);
		value = (T)(sInt ? sInt->value() : 0);
	}

	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value, void>::type
	lookup(const std::string& key, T &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!dynamic_cast<const SFloat *>(element) && strict)
		{
			throw std::runtime_error("Field '" + key + "' isn't numeric");
		}
		auto sFloat = dynamic_cast<const SFloat *>(element);
		value = (T)(sFloat ? sFloat->value() : 0.0);
	}

	template<typename T, typename _ = void>
	struct is_container : std::false_type
	{
	};

	template<typename... Ts>
	struct is_container_helper
	{
	};

	template<typename T>
	struct is_container<
		T,
		std::conditional_t<
			false,
			is_container_helper<
				typename T::value_type,
				typename T::size_type,
				typename T::allocator_type,
				typename T::iterator,
				typename T::const_iterator,
				decltype(std::declval<T>().size()),
				decltype(std::declval<T>().begin()),
				decltype(std::declval<T>().end()),
				decltype(std::declval<T>().cbegin()),
				decltype(std::declval<T>().cend())
			>,
			void
		>
	> : public std::true_type
	{
	};

	template<typename T>
	typename std::enable_if<is_container<T>::value, void>::type
	lookup(const std::string& key, T &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		auto arr = dynamic_cast<const SArr*>(element);
		if (arr)
		{
			arr->forEach([&value](const SVal* val) {
				value.emplace_back(*val);
			});
			return;
		}
		auto obj = dynamic_cast<const SObj*>(element);
		if (obj)
		{
			obj->forEach([&value](const std::string&, const SVal* val) {
				value.emplace_back(*val);
			});
			return;
		}
		if (strict)
		{
			throw std::runtime_error("Field '" + key + "' isn't appropriate container");
		}
	}

	template<typename T>
	typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value && !is_container<T>::value, void>::type
	lookup(const std::string& key, T &value, bool strict = false) const
	{
		const auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		value = (T)(element);
	}

	void lookup(const std::string& key, bool &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!dynamic_cast<SBool *>(element) && strict)
		{
			throw std::runtime_error("Field '" + key + "' isn't boolean");
		}
		value = (bool)(*element);
	}

	void lookup(const std::string& key, std::string &value, bool strict = false) const
	{
		auto element = get(key);
		if (!element)
		{
			throw std::runtime_error("Field '" + key + "' not found");
		}
		if (!dynamic_cast<SStr *>(element) && strict)
		{
			throw std::runtime_error("Field '" + key + "' isn't string");
		}
		value = std::remove_reference<decltype(value)>::type(*element);
	}

	template <typename T>
	inline void trylookup(const std::string& key, T &value) const noexcept
	{
		try	{ lookup(key, value, false); } catch (...) { value = T(); }
	}

	template< template<typename, typename, typename> class C, typename E, typename Cmp, typename A>
	void fill(C<E, Cmp, A> &container) const noexcept
	{
		forEach([&](const std::string& key, const SVal* value){
			container.emplace(key, value);
		});
	}

	template< template<typename, typename> class C, typename E, typename A>
	void fill(C<E, A> &container) const noexcept
	{
		forEach([&](const std::string&, const SVal* value){
			container.emplace_back(value);
		});
	}

	operator std::string() const override
	{
		std::ostringstream oss;
		oss << "[object#" << this << "(" << _elements.size() << ")]";
		return std::move(oss.str());
	};
	operator int() const override
	{
		return _elements.size();
	};
	operator double() const override
	{
		return _elements.size();
	};
	operator bool() const override
	{
		return !_elements.empty();
	};














};
