// Copyright © 2019 Dmitriy Khaustov. All rights reserved.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: khaustov.dm@gmail.com
// File created on: 2019.09.23

// LogHolder.hpp

#pragma once


#include <log/Log.hpp>

class LogHolder
{
protected:
	mutable Log _log;

public:
	LogHolder() = delete; // Default-constructor
	LogHolder(LogHolder&&) noexcept = delete; // Move-constructor
	LogHolder(const LogHolder&) = delete; // Copy-constructor
	~LogHolder() = default; // Destructor
	LogHolder& operator=(LogHolder&&) noexcept = delete; // Move-assignment
	LogHolder& operator=(LogHolder const&) = delete; // Copy-assignment

	LogHolder(
		const std::string& name,
		Log::Detail detail = Log::Detail::UNDEFINED,
		const std::string& sink = ""
	)
	: _log(name, detail, sink)
	{
	}

	Log& log() const
	{
		return _log;
	}
};


