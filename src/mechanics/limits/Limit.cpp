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
// File created on: 2017.09.17

// Limit.cpp


#include "Limit.hpp"
#include "LimitContainer.hpp"
#include "LimitConfig.hpp"
#include "LimitManager.hpp"

Limit::Limit(
	const std::shared_ptr<LimitContainer>& container,
	const Id& id,
	const Clarifier& clarifier
)
: id(id)
, clarifier(clarifier)
, _container(container)
, _config(LimitManager::get(id))
{
	if (!_config)
	{
		throw std::runtime_error("Not found config for limit '" + id + "'");
	}

	_value = _config->start;
	_expire = 0;

	_changed = false;
}

Limit::Limit(
	const std::shared_ptr<LimitContainer>& container,
	const Id& id,
	const Clarifier& clarifier,
	Value count,
	Time::Timestamp expire
)
: Limit(container, id, clarifier)
{
	_value = count;
	_expire = expire;

	_changed = false;
}

bool Limit::inDefaultState() const
{
	return _value == _config->start && (_expire == 0 || _expire == Time::interval(Time::Interval::ETERNITY));
}

void Limit::setChanged(bool isChanged)
{
	if (_changed == isChanged)
	{
		return;
	}

	_changed = isChanged;

	if (isChanged)
	{
		auto container = _container.lock();
		if (container)
		{
			container->setChanged();
		}
	}
}

bool Limit::change(int32_t delta, Time::Timestamp expire)
{
	if (_config->type == Type::Always || _config->type == Type::Never)
	{
		return false;
	}
	if (delta < 0)
	{
		delta = -std::min(abs(delta), abs(_value));
	}
	else if (delta > 0)
	{
		if (_config->type == Type::Loop)
		{
			delta = delta % (_config->max - _config->start);
			auto newCount = _value + delta;
			if (newCount >= _config->max)
			{
				newCount = newCount - _config->max + _config->start;
			}
			delta = abs(newCount) - abs(_value);
		}
		else
		{
			delta = std::min(abs(delta), abs(remain()));
		}
	}
	if (delta == 0 && expire == 0)
	{
		return false;
	}

	_value += delta;
	if (expire)
	{
		setExpire(expire);
	}
	else
	{
		initExpire();
	}

	setChanged();

	return true;
}

bool Limit::initExpire()
{
	if (_expire != 0)
	{
		return false;
	}

	Time::Timestamp expire;

	switch (_config->type)
	{
		case Type::None:
			expire = _config->duration ? (Time::now() + _config->duration) : Time::interval(Time::Interval::ETERNITY);
			break;

		case Type::Daily:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::DAY) + Time::interval(Time::Interval::DAY, 1),
				_config->duration ? (Time::now() + _config->duration) : Time::interval(Time::Interval::ETERNITY)
			);
			break;

		case Type::Weekly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::WEEK) + Time::interval(Time::Interval::WEEK, 1),
				_config->duration ? (Time::now() + _config->duration) : Time::interval(Time::Interval::ETERNITY)
			);
			break;

		case Type::Monthly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::MONTH) + Time::interval(Time::Interval::MONTH, 1),
				_config->duration ? (Time::now() + _config->duration) : Time::interval(Time::Interval::ETERNITY)
			);
			break;

		case Type::Yearly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::YEAR) + Time::interval(Time::Interval::YEAR, 1),
				_config->duration ? (Time::now() + _config->duration) : Time::interval(Time::Interval::ETERNITY)
			);
			break;

//		case Type::Loop:
//		case Type::Always:
//		case Type::Never:
//
		default:
			expire = Time::interval(Time::Interval::ETERNITY);
			break;
	}

	return setExpire(expire);
}

bool Limit::setExpire(Time::Timestamp expire)
{
	if (_expire == expire)
	{
		return false;
	}

	_expire = expire;

	setChanged();
	return true;
}

bool Limit::reset()
{
	if (_value == _config->start && _expire == 0)
	{
		return false;
	}

	_value = _config->start;
	_expire = 0;

	setChanged();

	return true;
}

bool Limit::setToMax()
{
	if (_value == _config->max)
	{
		return false;
	}

	_value = _config->max;

	setChanged();

	return true;
}

Limit::Value Limit::remain() const
{
	switch (_config->type)
	{
		case Type::Always:	// Сразу достигнут. Вырожденный лимит-блокер
			return 0;

		case Type::Never:	// Никогда недостижим. Вырожденный пропускающий лимит
		case Type::Loop:	// Автоматический сброс при превышении максимума (max+1 => start+1)
			return 1;

		default:
			return _config->max - _value;
	}
}

bool Limit::available() const
{
	switch (_config->type)
	{
		case Type::Always:	// Сразу достигнут. Вырожденный лимит-блокер
			return false;

		case Type::Never:	// Никогда недостижим. Вырожденный пропускающий лимит
		case Type::Loop:	// Автоматический сброс при превышении максимума (max+1 => start+1)
			return true;

		default:
			return _config->max > _value;
	}
}

SObj* Limit::serialize() const
{
	auto data = std::make_unique<SObj>();

	data->emplace("id", id); // Id лимита
	if (!clarifier.empty())
	{
		data->emplace("clarifier", clarifier); // Уточнение
	}
	data->emplace("value", _value); // Текущее значение
	if (_expire)
	{
		data->emplace("expire", _expire);
	}

	return data.release();
}

std::string Limit::typeToString(Limit::Type type)
{
	switch (type)
	{
		case Type::None:	return "none";
		case Type::Daily:	return "daily";
		case Type::Weekly:	return "weekly";
		case Type::Monthly:	return "monthly";
		case Type::Yearly:	return "yearly";
		case Type::Loop:	return "loop";
		case Type::Always:	return "always";
		case Type::Never:	return "never";
		default: 			return "_unknown";
	}
}
