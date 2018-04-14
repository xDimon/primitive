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
// File created on: 2017.05.26

// UrlSerializer.cpp


#include <iterator>
#include <cstring>
#include <iomanip>
#include "UrlSerializer.hpp"
#include "../transport/http/HttpUri.hpp"

REGISTER_SERIALIZER(uri, UrlSerializer);

struct tokens: std::ctype<char>
{
    tokens(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        typedef std::ctype<char> cctype;
        static const cctype::mask *const_rc= cctype::classic_table();

        static cctype::mask rc[cctype::table_size];
        std::memcpy(rc, const_rc, cctype::table_size * sizeof(cctype::mask));

        rc['&'] = std::ctype_base::space;
        rc[' '] = std::ctype_base::print;
        return &rc[0];
    }
};

SVal UrlSerializer::decode(const std::string& data)
{
	_iss.str(data);
	_iss.imbue(std::locale(std::locale(), new tokens()));

	std::vector<std::string> pairs(std::istream_iterator<std::string>(_iss), std::istream_iterator<std::string>{});

	SObj obj;

	try
	{
		for (const auto& pair : pairs)
		{
			auto seperatorPos = pair.find('=');
			auto key = pair.substr(0, seperatorPos);
			auto val = pair.substr(seperatorPos + 1);

			emplace(obj, key, val);
		}
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't decode URI ← ") + exception.what());
	}

	return obj;
}

std::string UrlSerializer::encode(const SVal& value)
{
	try
	{
		encodeValue("", value);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't encode into URI ← ") + exception.what());
	}

	return std::move(_oss.str());
}

void UrlSerializer::emplace(SObj& parent, std::string& keyline, const std::string& val)
{
	std::string key;
	std::string::size_type beginKeyPos;
	std::string::size_type endKeyPos;
	if (keyline[0] == '[')
	{
		beginKeyPos = 1;
		endKeyPos = keyline.find(']');

		if (endKeyPos == std::string::npos)
		{
			throw std::runtime_error("Not found close brace");
		}

		key = keyline.substr(beginKeyPos, endKeyPos - 1);
		keyline = keyline.substr(endKeyPos + 1);
	}
	else
	{
		beginKeyPos = 0;
		endKeyPos = keyline.find('[');

		key = keyline.substr(beginKeyPos, endKeyPos);
		if (endKeyPos == std::string::npos)
		{
			keyline = "";
		}
		else
		{
			keyline = keyline.substr(endKeyPos);
		}
	}

	auto oKey = HttpUri::urldecode(key);
	if (keyline.length())
	{
		if (!parent.get(oKey).is<SObj>())
		{
			SObj object;

			emplace(object, keyline, val);

			parent.emplace(oKey, std::move(object));
		}
		else
		{
			emplace(const_cast<SObj&>(parent.get(oKey).as<SObj>()), keyline, val);
		}
	}
	else
	{
		auto oVal = decodeValue(HttpUri::urldecode(val));

		parent.emplace(oKey, oVal);
	}
}

SVal UrlSerializer::decodeValue(const std::string& strval)
{
	// Empty
	if (!strval.length())
	{
		return "";
	}

//	// Definitely not number
//	if (!isdigit(strval[0]) && strval[0] != '-' && strval[0] != '+')
//	{
		return strval;
//	}

	// Deeper recognizing
	std::istringstream iss(strval);
	auto p = iss.tellg();

	auto c = iss.peek();
	if (c == '-' || c == '+')
	{
		iss.ignore();
	}

	for (auto i = 0;;)
	{
		c = iss.get();
		if (!isdigit(c))
		{
			break;
		}
		if (++i > 19)
		{
			return new SStr(strval);
		}
	}

	// Looks like a integer

	// No more data - it's integer value
	if (c == -1)
	{
		uint64_t value = 0;

		iss.clear(iss.goodbit);
		iss.seekg(p);
		iss >> value;

		return new SInt(value);
	}

	if (c != '.' && c != 'e' && c != 'E')
	{
		return new SStr(strval);
	}
	iss.ignore();

	c = iss.peek();
	if (c == '-' || c == '+')
	{
		iss.ignore();
	}

	for (;;)
	{
		c = iss.get();
		if (!isdigit(c))
		{
			break;
		}
	}

	// Looks like a float

	// No more data - it's float value
	if (c == -1)
	{
		SFloat::type value = 0;

		iss.clear(iss.goodbit);
		iss.seekg(p);
		iss >> value;

		return new SFloat(value);
	}

	// Remain data - it's not number

	return new SStr(strval);
}

void UrlSerializer::encodeNull(const std::string& keyline, const SVal&)
{
	_oss << keyline << "=" << "*null*";
}

void UrlSerializer::encodeBool(const std::string& keyline, const SVal& value)
{
	_oss << keyline << "=" << (value.as<SBool>().value() ? "*true*" : "*false*");
}

void UrlSerializer::encodeString(const std::string& keyline, const SVal& value)
{
	_oss << keyline << "=" << HttpUri::urlencode(value.as<SStr>().value());
}

void UrlSerializer::encodeBinary(const std::string& keyline, const SVal& value)
{
	_oss << "*binary*";
}

void UrlSerializer::encodeNumber(const std::string& keyline, const SVal& value)
{
	if (value.is<SInt>())
	{
		_oss << keyline << "=" << value.as<SInt>().value();
		return;
	}
	if (value.is<SFloat>())
	{
		_oss << keyline << "=" << std::setprecision(15) << value.as<SFloat>().value();
		return;
	}
}

void UrlSerializer::encodeArray(const std::string& keyline, const SVal& value)
{
	auto& array = value.as<SArr>();

	int index = 0;
	std::for_each(array.cbegin(), array.cend(),
		[this, &keyline, &index]
		(const SVal& element)
		{
			if (index > 0)
			{
				_oss << "&";
			}
			std::ostringstream oss;
			oss << keyline << "[" << index++ << "]";
			encodeValue(oss.str(), element);
		}
	);
}

void UrlSerializer::encodeObject(const std::string& keyline, const SVal& value)
{
	auto& object = value.as<SObj>();

	bool empty = true;
	std::for_each(object.cbegin(), object.cend(),
		[&]
		(const std::pair<std::string, SVal>& element)
		{
			if (!empty)
			{
				_oss << "&";
			}
			else
			{
				empty = false;
			}
			std::ostringstream oss;
			oss << keyline << (keyline.empty() ? "" : "[") << element.first << (keyline.empty() ? "" : "]");
			encodeValue(oss.str(), element.second);
		}
	);
}

void UrlSerializer::encodeValue(const std::string& keyline, const SVal& value)
{
	if (value.is<SStr>())
	{
		encodeString(keyline, value);
	}
	else if (value.is<SInt>())
	{
		encodeNumber(keyline, value);
	}
	else if (value.is<SFloat>())
	{
		encodeNumber(keyline, value);
	}
	else if (value.is<SObj>())
	{
		encodeObject(keyline, value);
	}
	else if (value.is<SArr>())
	{
		encodeArray(keyline, value);
	}
	else if (value.is<SBool>())
	{
		encodeBool(keyline, value);
	}
	else if (value.is<SNull>())
	{
		encodeNull(keyline, value);
	}
	else if (value.is<SBinary>())
	{
		encodeBinary(keyline, value);
	}
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}
