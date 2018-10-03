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
// File created on: 2017.11.19

// ProtobufSerializer.cpp


#include "ProtobufSerializer.hpp"
#include "SObj.hpp"
#include "SBinary.hpp"

#include <iomanip>

REGISTER_SERIALIZER(protobuf, ProtobufSerializer);

template <class T>
typename std::enable_if<std::is_base_of<T, google::protobuf::Message>::value, SVal*>::type
ProtobufSerializer::decode(const std::string& data)
{
	try
	{
		T msg{};
		msg.ParseFromString(data);
		return decodeMessage(msg);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error("Can't decode from Protobuf-message");
	}
}

SVal* ProtobufSerializer::decode(const std::string& data)
{
	throw std::runtime_error("Can't decode from Protobuf-string ← Not implemented");
}

SVal* ProtobufSerializer::decodeMessage(const google::protobuf::Message& msg)
{
	const google::protobuf::Descriptor *d = msg.GetDescriptor();
	const google::protobuf::Reflection *ref = msg.GetReflection();
	if (!d || !ref)
	{
		return 0; // TODO exception
	}

	auto root = std::make_unique<SObj>();

	std::vector<const google::protobuf::FieldDescriptor *> fields;
	ref->ListFields(msg, &fields);

	for (size_t i = 0; i != fields.size(); i++)
	{
		const google::protobuf::FieldDescriptor *field = fields[i];

		const std::string &name = (field->is_extension()) ? field->full_name() : field->name();

		if(field->is_repeated())
		{
			auto count = ref->FieldSize(msg, field);
			if (!count)
			{
				continue;
			}

			auto arr = std::make_unique<SArr>();

			for (int j = 0; j < count; j++)
			{
				arr->insert(decodeField(msg, field, j));
			}

			root->insert(name.c_str(), arr.release());
		}
		else if (ref->HasField(msg, field))
		{
			root->insert(name.c_str(), decodeField(msg, field, 0));
		}
		else
		{
			continue;
		}
	}

	return root.release();
}

SVal* ProtobufSerializer::decodeField(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor *field, int index)
{
	const google::protobuf::Reflection* ref = msg.GetReflection();
	const bool repeated = field->is_repeated();

	switch (field->cpp_type())
	{
#define _CONVERT(type, ctype, sfunc, afunc)                          \
        case google::protobuf::FieldDescriptor::type:                \
            if (repeated)                                            \
            {                                                        \
                return new ctype(ref->afunc(msg, field, index));     \
            }                                                        \
            else                                                     \
            {                                                        \
                return new ctype(ref->sfunc(msg, field));            \
            }                                                        \
            break;                                                   \

		_CONVERT(CPPTYPE_DOUBLE, SFloat, GetDouble, GetRepeatedDouble);
		_CONVERT(CPPTYPE_FLOAT, SFloat, GetFloat, GetRepeatedFloat);
		_CONVERT(CPPTYPE_INT64, SInt, GetInt64, GetRepeatedInt64);
		_CONVERT(CPPTYPE_UINT64, SInt, GetUInt64, GetRepeatedUInt64);
		_CONVERT(CPPTYPE_INT32, SInt, GetInt32, GetRepeatedInt32);
		_CONVERT(CPPTYPE_UINT32, SInt, GetUInt32, GetRepeatedUInt32);
		_CONVERT(CPPTYPE_BOOL, SBool, GetBool, GetRepeatedBool);
#undef _CONVERT

		case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
		{
			std::string scratch;
			const std::string& value =
				repeated ?
				ref->GetRepeatedStringReference(msg, field, index, &scratch) :
				ref->GetStringReference(msg, field, &scratch);

			if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
			{
				return new SBinary(value);
			}
			else
			{
				return new SStr(value);
			}
		}

		case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
			if (repeated)
			{
				return decodeMessage(ref->GetRepeatedMessage(msg, field, index));
			}
			else
			{
				return decodeMessage(ref->GetMessage(msg, field));
			}

		case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
			if (repeated)
			{
				return new SInt(ref->GetRepeatedEnum(msg, field, index)->number());
			}
			else
			{
				return new SInt(ref->GetEnum(msg, field)->number());
			}

		default:
			throw std::runtime_error("Unsupported type of field");
	}
}

std::string ProtobufSerializer::encode(const SVal* value)
{
	throw std::runtime_error("Can't encode into Protobuf ← Not implemented");
}
