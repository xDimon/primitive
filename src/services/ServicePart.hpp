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
// File created on: 2017.07.02

// ServicePart.hpp


#pragma once


#include "../utils/Shareable.hpp"
#include "../utils/Named.hpp"
#include "../log/Log.hpp"
#include "../configs/Setting.hpp"

class Service;

class ServicePart : public Shareable<ServicePart>, public Named
{
protected:
	std::weak_ptr<Service> _service;
	mutable Log _log;

public:
	ServicePart() = delete;
	ServicePart(const ServicePart&) = delete;
	ServicePart& operator=(ServicePart const&) = delete;
	ServicePart(ServicePart&&) noexcept = delete;
	ServicePart& operator=(ServicePart&&) noexcept = delete;

	explicit ServicePart(const std::shared_ptr<Service>& service);
	~ServicePart() override = default;

	virtual void init(const Setting& setting) {};
	virtual void postInit() {};

	Log& log()
	{
		return _log;
	}

	template <class Service>
	std::shared_ptr<Service> service()
	{
		return std::dynamic_pointer_cast<Service>(_service.lock());
	}

	class Comparator
	{
	public:
		bool operator()(const std::shared_ptr<ServicePart>& left, const std::shared_ptr<ServicePart>& right) const
		{
			return left->name() < right->name();
		}
	};
};
