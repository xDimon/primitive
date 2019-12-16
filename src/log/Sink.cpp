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
// File created on: 2017.08.16

// Sink.cpp


#include <stdexcept>
#include "Sink.hpp"
#include "../utils/Time.hpp"
#include "../thread/Thread.hpp"
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <utility>
#include <climits>
#include <unistd.h>

#if __cplusplus < 201703L
#define constexpr
#endif

static const char *levelLabel[] = {
	"TRC",
	"DEB",
	"INF",
	"WRN",
	"ERR",
	"CRT"
};

const size_t Sink::accumucatorCapacity = 1u << 14u;

Sink::Sink(std::string name, const Setting& setting)
: _name(std::move(name))
, _f(nullptr)
{
	std::string type;
	try
	{
		type = setting.getAs<SStr>("type").value();
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error("Can't get type of sink '" + _name + "' ← " + exception.what());
	}

	if (type == "console")
	{
		_type = Type::CONSOLE;
	}
	else if (type == "file")
	{
		_type = Type::FILE;
	}
	else if (type == "null")
	{
		_type = Type::BLACKHOLE;
	}
	else
	{
		throw std::runtime_error("Unknown type ('" + type + "') of sink '" + _name + "'");
	}

	if (_type == Type::FILE)
	{
		std::string directory;
		try
		{
			directory = setting.getAs<SStr>("directory");
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error("Can't get directory for sink '" + _name + "' ← " + exception.what());
		}
		if (directory.empty() || directory[0] != '/')
		{
			char path[PATH_MAX];
			directory = getcwd(path, sizeof(path));
		}

		std::string filename;
		try
		{
			filename = setting.getAs<SStr>("filename");
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error("Can't get filename of sink '" + _name + "' ← " + exception.what());
		}
		if (filename.empty() || std::find_if(filename.begin(), filename.end(), [](const auto& s){ return s == '/'; }) != filename.end())
		{
			throw std::runtime_error("Wrong filename of sink '" + _name + "' ← Bad filename");
		}

		_path = directory + "/" + filename;
		_f = fopen(_path.c_str(), "a+");

		if (_f == nullptr)
		{
			throw std::runtime_error("Can't open log-file (" + _path + ") ← " + strerror(errno));
		}
	}
	else if (_type == Type::CONSOLE)
	{
		_f = stdout;
	}
}

Sink::Sink()
: _type(Type::BLACKHOLE)
, _f(nullptr)
{
}

Sink::~Sink()
{
	if (_type == Type::FILE)
	{
		if (_f)
		{
			fclose(_f);
			_f = nullptr;
		}
	}
}

void Sink::push(Log::Detail logLevel, const std::string& name, const std::string& message)
{
	const char* threadLabel = Thread::self()->name().c_str();

	auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	time_t ts = now / 1'000'000;
	auto msec = static_cast<uint32_t>(now % 1'000'000 / 1000);
	auto usec = static_cast<uint32_t>(now % 1'000);

	tm tm{};
	localtime_r(&ts, &tm);

	int headerSize = 0;

	char buff[1<<6]; // 64
	headerSize = snprintf(buff, sizeof(buff),
		"%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
		tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		msec, usec,
		threadLabel, name.c_str(), levelLabel[static_cast<int>(logLevel)]
	);

	std::unique_lock<mutex_t> lock(_mutex);

	if (_f == nullptr)
	{
		_preInitBuff += buff + message + "\n";
	}
	else
	{
		if constexpr (accumucatorCapacity > 0)
		{
			if ((_accumulator.length() + headerSize + message.length() + 1) > accumucatorCapacity)
			{
				lock.unlock();
				flush();
				lock.lock();
			}
			_accumulator.append(buff);
			_accumulator.append(message);
			_accumulator.append("\n");

			if (!_flushTimeout)
			{
				_flushTimeout = std::make_shared<Timer>(
					[wp = std::weak_ptr<Sink>(std::dynamic_pointer_cast<Sink>(ptr()))]
					{
						try { if (auto iam = wp.lock()) iam->flush(); } catch (...) {}
					},
					"Timeout to flush sink"
				);
			}

			_flushTimeout->startOnce(std::chrono::milliseconds(250));
		}
		else
		{
			fwrite(buff, (headerSize > 0) ? static_cast<size_t>(headerSize) : 0, 1, _f);
			fwrite(message.data(), message.size(), 1, _f);
			fputc('\n', _f);
			fflush(_f);
		}
	}
}

void Sink::push(Log::Detail level, const std::string& name, const std::string& format, va_list ap)
{
	auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	time_t ts = now / 1'000'000;
	auto msec = static_cast<uint32_t>(now % 1'000'000 / 1000);
	auto usec = static_cast<uint32_t>(now % 1'000);

	tm tm{};
	localtime_r(&ts, &tm);

	const char* threadLabel = Thread::self()->name().c_str();

	if (_f == nullptr)
	{
		char buff[1<<6]; // 64

		snprintf(buff, sizeof(buff),
			"%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
			tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			msec, usec,
			threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);

		auto size = format.size() * 2 + 64;
		std::string message;
		for (;;)
		{
			message.resize(size);
			va_list ap1;
			va_copy(ap1, ap);
			int n = vsnprintf(const_cast<char*>(message.data()), size, format.c_str(), ap1);
			va_end(ap1);
			if (n > -1)
			{
				if (static_cast<size_t>(n) < size)
				{
					// Everything worked
					message.resize(static_cast<size_t>(n));
					break;
				}
				size = static_cast<size_t>(n) + 1;
			}
			else
			{
				size *= 2;
			}
		}

		std::lock_guard<mutex_t> lockGuard(_mutex);

		_preInitBuff += buff + message + "\n";
	}
	else if constexpr (accumucatorCapacity > 0)
	{
		char buff[1<<6]; // 64

		int headerSize = snprintf(buff, sizeof(buff),
			"%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
			tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			msec, usec,
			threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);

		auto size = format.size() * 2 + 64;
		std::string message;
		for (;;)
		{
			message.resize(size);
			va_list ap1;
			va_copy(ap1, ap);
			int n = vsnprintf(const_cast<char*>(message.data()), size, format.c_str(), ap1);
			va_end(ap1);
			if (n > -1)
			{
				if (static_cast<size_t>(n) < size)
				{
					// Everything worked
					message.resize(static_cast<size_t>(n));
					break;
				}
				size = static_cast<size_t>(n) + 1;
			}
			else
			{
				size *= 2;
			}
		}

		{
			std::unique_lock<mutex_t> lock(_mutex);

			if ((_accumulator.length() + headerSize + message.length() + 1) > accumucatorCapacity)
			{
				lock.unlock();
				flush();
				lock.lock();
			}
			_accumulator.append(buff);
			_accumulator.append(message);
			_accumulator.append("\n");

			if (!_flushTimeout)
			{
				_flushTimeout = std::make_shared<Timer>(
					[wp = std::weak_ptr<Sink>(std::dynamic_pointer_cast<Sink>(ptr()))]
					{
						try { if (auto iam = wp.lock()) iam->flush(); } catch (...) {}
					},
					"Timeout to flush sink"
				);
			}
		}

		_flushTimeout->startOnce(std::chrono::milliseconds(250));
	}
	else
	{
		std::lock_guard<mutex_t> lockGuard(_mutex);

		fprintf(_f, "%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
			tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			msec, usec,
			threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);
		vfprintf(_f, format.c_str(), ap);
		fputc('\n', _f);
		fflush(_f);
	}
}

// Принудительно сбросить на диск
void Sink::flush()
{
	std::lock_guard<mutex_t> lockGuard(_mutex);

	if (_f == nullptr)
	{
		return;
	}

	if constexpr (accumucatorCapacity > 0)
	{
		if (_accumulator.empty())
		{
			return;
		}

		fwrite(_accumulator.data(), _accumulator.size(), 1, _f);

		_accumulator.clear();
	}

	fflush(_f);
}

void Sink::rotate()
{
	if (_type == Type::FILE)
	{
		auto f = fopen(_path.c_str(), "a+");
		if (f == nullptr)
		{
			push(Log::Detail::INFO, "Logger", "Close logfile for rotation");
			throw std::runtime_error("Can't reopen log-file (" + _path + ") ← " + strerror(errno));
		}

		push(Log::Detail::INFO, "Logger", "Close logfile for rotation");

		{
			std::lock_guard<mutex_t> lockGuard(_mutex);

			if (_f != nullptr)
			{
				fflush(_f);
				fclose(_f);
			}

			_f = f;
		}

		push(Log::Detail::INFO, "Logger", "Reopen logfile for rotation");
	}
}
