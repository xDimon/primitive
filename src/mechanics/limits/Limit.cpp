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
// File created on: 2017.09.17

// Limit.cpp


#include "Limit.hpp"
#include "LimitConfig.hpp"
#include "LimitManager.hpp"

Limit::Limit(
	const Id& id,
	const Clarifier& clarifier
)
: id(id)
, clarifier(clarifier)
, _config(LimitManager::get(id))
{
	if (!_config)
	{
		throw std::runtime_error("Not found config for limit '" + id + "'");
	}

	_value = _config->start;
	switch (_config->type)
	{
		case Type::Daily:
			_expire = Time::trim(Time::now(), Time::Interval::DAY) + Time::interval(Time::Interval::DAY, 1) + _config->offset;
			break;

		case Type::Weekly:
			_expire = Time::trim(Time::now(), Time::Interval::WEEK) + Time::interval(Time::Interval::WEEK, 1) + _config->offset;
			break;

		case Type::Monthly:
			_expire = Time::trim(Time::now(), Time::Interval::MONTH) + Time::interval(Time::Interval::MONTH, 1) + _config->offset;
			break;

		case Type::Yearly:
			_expire = Time::trim(Time::now(), Time::Interval::YEAR) + Time::interval(Time::Interval::YEAR, 1) + _config->offset;
			break;

		case Type::None:
		case Type::Loop:
		case Type::Always:
		case Type::Never:

		default:
			_expire = Time::interval(Time::Interval::ETERNITY);
			break;
	}

	_changed = false;
}

Limit::Limit(
	const Id& id,
	const Clarifier& clarifier,
	Value count,
	Time::Timestamp expire
)
: Limit(id, clarifier)
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
}

bool Limit::change(int32_t delta)
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
	if (delta == 0)
	{
		return false;
	}

	_value += delta;
	expireInit();

	setChanged();

	return true;
}

bool Limit::expireInit()
{
	if (_expire != 0)
	{
		return false;
	}

	Time::Timestamp expire;

	switch (_config->type)
	{
		case Type::None:
			expire = Time::now() + _config->duration;
			break;

		case Type::Daily:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::DAY) + Time::interval(Time::Interval::DAY, 1),
				Time::now() + _config->duration
			);
			break;

		case Type::Weekly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::WEEK) + Time::interval(Time::Interval::WEEK, 1),
				Time::now() + _config->duration
			);
			break;

		case Type::Monthly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::MONTH) + Time::interval(Time::Interval::MONTH, 1),
				Time::now() + _config->duration
			);
			break;

		case Type::Yearly:
			expire = std::min(
				Time::trim(Time::now(), Time::Interval::YEAR) + Time::interval(Time::Interval::YEAR, 1),
				Time::now() + _config->duration
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

	data->insert("id", id); // Id лимита
	data->insert("clarifier", clarifier); // Уточнение
	data->insert("type", typeToString(_config->type)); // Тип поведения
	data->insert("start", _config->start); // Начальное значение
	data->insert("max", _config->max); // Максимальное значение
	data->insert("offset", _config->offset); // Сдвиг для фиксированных сбросов (Day, Week, Month, Year)
	data->insert("duration", _config->duration); // Продолжительность
	data->insert("value", _value); // Текущее значение
	data->insert("expire", _expire);

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