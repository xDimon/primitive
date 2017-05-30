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
// File created on: 2017.05.26

// UrlSerializer.cpp


#include <iterator>
#include <cstring>
#include <memory>
#include <iomanip>
#include "UrlSerializer.hpp"
#include "../transport/http/HttpUri.hpp"
#include "../utils/Base64.hpp"

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

SVal* UrlSerializer::decode(const std::string& data, bool strict)
{
	_iss.str(data);
	_iss.imbue(std::locale(std::locale(), new tokens()));

	std::vector<std::string> pairs(std::istream_iterator<std::string>(_iss), std::istream_iterator<std::string>{});

	try
	{
		auto obj = std::make_unique<SObj>();

		for (auto pair : pairs)
		{
			auto seperatorPos = pair.find('=');
			auto key = pair.substr(0, seperatorPos);
			auto val = pair.substr(seperatorPos + 1);

			emplace(obj.get(), key, val);

			continue;
		}

		return obj.release();
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't decode URI: ") + exception.what());
	}
}

std::string UrlSerializer::encode(const SVal* value)
{
	try
	{
		encodeValue("", value);
	}
	catch (std::runtime_error& exception)
	{
		throw std::runtime_error(std::string("Can't encode into URI: ") + exception.what());
	}

	return std::move(_oss.str());
}

void UrlSerializer::emplace(SObj* parent, std::string& keyline, const std::string& val)
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

	auto oKey = std::make_unique<SStr>(HttpUri::urldecode(key));
	if (keyline.length())
	{
		auto t = oKey.get();
		auto a = parent->get(t);
		SObj* object = dynamic_cast<SObj*>(a);
		if (!object)
		{
			object = new SObj;
			parent->insert(oKey.release(), object);
		}

		emplace(object, keyline, val);
	}
	else
	{
		auto oVal = std::unique_ptr<SVal>(decodeValue(HttpUri::urldecode(val)));

		parent->insert(oKey.release(), oVal.release());
	}
}

SVal* UrlSerializer::decodeValue(const std::string& strval)
{
	// Empty
	if (!strval.length())
	{
		return new SStr();
	}

	// Definitely not number
	if (!isdigit(strval[0]) && strval[0] != '-' && strval[0] != '+')
	{
		return new SStr(strval);
	}

	// Deeper recognizing
	std::istringstream iss(strval);
	auto p = iss.tellg();

	while (!iss.eof())
	{
		auto c = iss.get();
		if (c == '.' || c == 'e' || c == 'E')
		{
			// Looks like a float
			SFloat::type value = 0;

			iss.seekg(p);
 			iss >> value;

			// Remain data - it's not number
			if (iss.get() != -1)
			{
				return new SStr(strval);
			}

			// No more data - it's float value
			return new SFloat(value);
		}
		else if (!isdigit(c) && c != '-')
		{
			break;
		}
	}

	// Looks like a integer
	SInt::type value = 0;

	iss.seekg(p);
	iss >> value;

	// Remain data - it's not number
	if (iss.get() != -1)
	{
		return new SStr(strval);
	}

	// No more data - it's integer value
	return new SInt(value);
}

void UrlSerializer::encodeNull(const std::string& keyline, const SNull* value)
{
	_oss << keyline << "=" << "*null*";
}

void UrlSerializer::encodeBool(const std::string& keyline, const SBool* value)
{
	_oss << keyline << "=" << (value->value() ? "*true*" : "*false*");
}

void UrlSerializer::encodeString(const std::string& keyline, const SStr* value)
{
	_oss << keyline << "=" << HttpUri::urlencode(value->value());
}

void UrlSerializer::encodeBinary(const std::string& keyline, const SBinary* value)
{
	_oss << "*binary*";
}

void UrlSerializer::encodeNumber(const std::string& keyline, const SNum* value)
{
	const SInt* intVal = dynamic_cast<const SInt*>(value);
	if (intVal)
	{
		_oss << keyline << "=" << intVal->value();
		return;
	}
	const SFloat* floatVal = dynamic_cast<const SFloat*>(value);
	if (floatVal)
	{
		_oss << keyline << "=" << std::setprecision(15) << floatVal->value();
		return;
	}
}

void UrlSerializer::encodeArray(const std::string& keyline, const SArr* value)
{
	int index = 0;
	value->forEach([this,&keyline,&index](const SVal* value)
	{
		if (index > 0)
		{
			_oss << "&";
		}
		std::ostringstream oss;
		oss << keyline << "[" << index++ << "]";
		encodeValue(oss.str(), value);
	});
}

void UrlSerializer::encodeObject(const std::string& keyline, const SObj* value)
{
	bool empty = true;
	value->forEach([this,&keyline,&empty](const std::pair<const SStr* const, SVal*>& element)
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
		oss << keyline << (keyline.empty()?"":"[") << element.first->value() << (keyline.empty()?"":"]");
		encodeValue(oss.str(), element.second);
	});
}

void UrlSerializer::encodeValue(const std::string& keyline, const SVal* value)
{
	if (auto pStr = dynamic_cast<const SStr*>(value))
	{
		encodeString(keyline, pStr);
	}
	else if (auto pNum = dynamic_cast<const SNum*>(value))
	{
		encodeNumber(keyline, pNum);
	}
	else if (auto pObj = dynamic_cast<const SObj*>(value))
	{
		encodeObject(keyline, pObj);
	}
	else if (auto pArr = dynamic_cast<const SArr*>(value))
	{
		encodeArray(keyline, pArr);
	}
	else if (auto pBool = dynamic_cast<const SBool*>(value))
	{
		encodeBool(keyline, pBool);
	}
	else if (auto pNull = dynamic_cast<const SNull*>(value))
	{
		encodeNull(keyline, pNull);
	}
	else if (auto pBin = dynamic_cast<const SBinary*>(value))
	{
		encodeBinary(keyline, pBin);
	}
	else
	{
		throw std::runtime_error("Unknown value type");
	}
}