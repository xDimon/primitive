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

Sink::Sink(const Setting& setting)
{
	_f = nullptr;

	if (!setting.lookupValue("name", _name))
	{
		throw std::runtime_error("Not found name for one of sinks");
	}

	std::string type;
	if (!setting.lookupValue("type", type))
	{
		throw std::runtime_error("Undefined type for sink '" + _name + "'");
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
		throw std::runtime_error("Unknown type ('" + type + "') for sink '" + _name + "'");
	}

	if (_type == Type::FILE)
	{
		std::string directory;
		if (!setting.lookupValue("directory", directory))
		{
			throw std::runtime_error("Undefined directory for sink '" + _name + "'");
		}
		if (directory.empty() || directory[0] != '/')
		{
			throw std::runtime_error("Invalid directory for sink '" + _name + "': path must be absolute");
		}

		std::string filename;
		if (!setting.lookupValue("filename", filename))
		{
			throw std::runtime_error("Undefined filename for sink '" + _name + "'");
		}
		if (filename.empty() || std::find_if(filename.begin(), filename.end(), [](const auto& s){ return s == '/'; }) != filename.end())
		{
			throw std::runtime_error("Invalid filename for sink '" + _name + "'");
		}

		_path = directory + "/" + filename;
		_f = fopen(_path.c_str(), "a+");

		if (_f == nullptr)
		{
			throw std::runtime_error("Can't open log-file (" + _path + "): " + strerror(errno));
		}
	}
	else if (_type == Type::CONSOLE)
	{
		_f = stdout;
	}
}

Sink::Sink()
{
	_type = Type::BLACKHOLE;
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
	char buff[1<<6]; // 64

	time_t ts;
	struct tm tm;

	auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	ts = now / 1'000'000;
	auto ms = static_cast<uint32_t>(now % 1'000'000 / 1000);
	auto mcs = static_cast<uint32_t>(now % 1'000);

	localtime_r(&ts, &tm);

	const char* threadLabel = Thread::self()->name().c_str();

	static const char *levelLabel[] = {
		"TRC",
		"DEB",
		"INF",
		"WRN",
		"ERR",
		"CRT"
	};

	int n = 0;

	n = snprintf(buff, sizeof(buff),
		"%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
		tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		ms, mcs,
		threadLabel, name.c_str(), levelLabel[static_cast<int>(logLevel)]
	);

	if (_f == nullptr)
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		_preInitBuff += buff + message + "\n";
	}
	else
	{
#ifdef	PRE_ACCUMULATE_LOG
		_mutex.lock();
		if ((_accum.length() + n + message.length() + 1) > (1<<14))
		{
			_mutex.unlock();
			flush();
			_mutex.lock();
		}
		_accum.append(buff);
		_accum.append(message);
		_accum.append("\n");
		_mutex.unlock();
#else
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		fwrite(buff, n, 1, _f);
		fwrite(message.data(), message.size(), 1, _f);
		fputc('\n', _f);
		fflush(_f);
#endif
	}
}

void Sink::push(Log::Detail level, const std::string& name, const std::string& format, va_list ap)
{
	time_t ts;
	struct tm tm;

	auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	ts = now / 1'000'000;
	auto ms = static_cast<uint32_t>(now % 1'000'000 / 1000);
	auto mcs = static_cast<uint32_t>(now % 1'000);

	localtime_r(&ts, &tm);

	const char* threadLabel = Thread::self()->name().c_str();

	static const char *levelLabel[] = {
		"TRC",
		"DEB",
		"INF",
		"WRN",
		"ERR",
		"CRT"
	};

	if (_f == nullptr)
	{
		char buff[1<<6]; // 64

		snprintf(buff, sizeof(buff),
			 "%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
			 tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
			 tm.tm_hour, tm.tm_min, tm.tm_sec,
			 ms, mcs,
			 threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);

		int size = ((int)format.size()) * 2 + 50;   // Use a rubric appropriate for your code
		std::string message;
		while (1) // Maximum two passes on a POSIX system...
		{
			message.resize(size);
			va_list ap1;
			va_copy(ap1, ap);
			int n = vsnprintf((char *)message.data(), size, format.c_str(), ap1);
			va_end(ap1);
			if (n > -1 && n < size)
			{  // Everything worked
				message.resize(n);
				break;
			}
			if (n > -1)  // Needed size returned
			{
				size = n + 1;   // For null char
			}
			else
			{
				size *= 2;      // Guess at a larger size (OS specific)
			}
		}

		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

		_preInitBuff += buff + message + "\n";
	}
	else
	{
#ifdef	PRE_ACCUMULATE_LOG
		char buff[1<<6]; // 64

		int n = snprintf(buff, sizeof(buff),
				 "%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
				 tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
				 tm.tm_hour, tm.tm_min, tm.tm_sec,
				 ms, mcs,
				 threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);

		int size = ((int)format.size()) * 2 + 64;
		std::string message;
		for (;;) // Maximum two passes on a POSIX system...
		{
			message.resize(size);
			va_list ap1;
			va_copy(ap1, ap);
			int n = vsnprintf((char *)message.data(), size, format.c_str(), ap1);
			va_end(ap1);
			if (n > -1 && n < size)
			{  // Everything worked
				message.resize(n);
				break;
			}
			if (n > -1)  // Needed size returned
			{
				size = n + 1;   // For null char
			}
			else
			{
				size *= 2;      // Guess at a larger size (OS specific)
			}
		}

		{
			std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

			if ((_accum.length() + n + message.length() + 1) > (1<<14))
			{
				_mutex.unlock();
				flush();
				_mutex.lock();
			}
			_accum.append(buff);
			_accum.append(message);
			_accum.append("\n");

			if (!_flushTimeout)
			{
				_flushTimeout = std::make_shared<Timer>(
					[wp = std::weak_ptr<Sink>(std::dynamic_pointer_cast<Sink>(ptr()))]
					{
						auto iam = wp.lock();
						if (!iam) return;
						try
						{
							iam->flush();
							iam->_flushTimeout->startOnce(std::chrono::milliseconds(250));
						} catch (...) {}
					},
					"Timeout to flush sink"
				);
			}
		}

		_flushTimeout->startOnce(std::chrono::milliseconds(250));
#else
		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);
		fprintf(_f, "%02u-%02u-%02u %02u:%02u:%02u.%03u'%03u\t%s\t%s\t%s\t",
			tm.tm_year%100, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			ms, mcs,
			threadLabel, name.c_str(), levelLabel[static_cast<int>(level)]
		);
		vfprintf(_f, format.c_str(), ap);
		fputc('\n', _f);
		fflush(_f);
#endif
	}
}

// Принудительно сбросить на диск
void Sink::flush()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

#ifdef	PRE_ACCUMULATE_LOG
	if (_accum.empty())
	{
		return;
	}
#endif

	if (_f == nullptr)
	{
		return;
	}

#ifdef	PRE_ACCUMULATE_LOG
	std::string chunk = std::move(_accum);

	fwrite(chunk.data(), chunk.size(), 1, _f);
#endif

	fflush(_f);
}

void Sink::rotate()
{
	if (_type == Type::FILE)
	{
		push(Log::Detail::INFO, "Logger", "Close logfile for rotation");

		std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

		auto f = fopen(_path.c_str(), "a+");
		if (f == nullptr)
		{
			throw std::runtime_error("Can't reopen log-file (" + _path + "): " + strerror(errno));
		}

		push(Log::Detail::INFO, "Logger", "Reopen logfile for rotation");

		if (_f != nullptr)
		{
			fflush(_f);
			fclose(_f);
		}

		_f = f;
	}
}
