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

// ProtobufSerializer.hpp


#pragma once

#include <google/protobuf/message.h>
#include "Serializer.hpp"

class ProtobufSerializer : public Serializer
{
	DECLARE_SERIALIZER(ProtobufSerializer);

public:
	template <class T>
	typename std::enable_if<std::is_base_of<T, google::protobuf::Message>::value, SVal>::type
	decode(const std::string& data);

	SVal decodeMessage(const google::protobuf::Message& msg);

private:
	SVal decodeField(const google::protobuf::Message& msg, const google::protobuf::FieldDescriptor* field, int index);
};
