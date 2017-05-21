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
// File created on: 2017.04.21

// LoggerManager.cpp


#include <stdexcept>
#include <thread>
#include "LoggerManager.hpp"

LoggerManager::LoggerManager()
{
    //create P7 client object
    _logClient = P7_Create_Client("/P7.Sink=Console /P7.Pool=32768 /P7.Format=%tm\t%tn\t%mn\t%lv\t%ms");
    if (!_logClient)
    {
        throw std::runtime_error("Can't create p7-client");
    }

    //create P7 trace object 1
    _logTrace = P7_Create_Trace(_logClient, "TraceChannel");
    if (!_logTrace)
    {
		throw std::runtime_error("Can't create p7-channel");
    }
	_logTrace->Share("TraceChannel");

	uint32_t tid = 0;//static_cast<uint32_t>(((pthread_self() >> 32) ^ pthread_self()) & 0xFFFFFFFF);
	_logTrace->Register_Thread("MainThread", tid);

//	P7_Set_Crash_Handler();
}

LoggerManager::~LoggerManager()
{
    if (_logTrace)
    {
        _logTrace->Release();
        _logTrace = nullptr;
    }

    if (_logClient)
    {
        _logClient->Release();
        _logClient = nullptr;
    }
}

bool LoggerManager::regThread(std::string threadName)
{
	if (getInstance()._logTrace)
	{
		uint32_t tid = 0;//static_cast<uint32_t>(((pthread_self() >> 32) ^ pthread_self()) & 0xFFFFFFFF);
		return getInstance()._logTrace->Register_Thread(threadName.c_str(), tid) != 0;
	}
	return false;
}

bool LoggerManager::unregThread()
{
	uint32_t tid = 0;//static_cast<uint32_t>(((pthread_self() >> 32) ^ pthread_self()) & 0xFFFFFFFF);
	return getInstance()._logTrace->Unregister_Thread(tid) != 0;
}


IP7_Trace *LoggerManager::getLogTrace()
{
	std::lock_guard<std::recursive_mutex> lockGuard(getInstance()._mutex);
	if (getInstance()._logTrace)
	{
		return P7_Get_Shared_Trace("TraceChannel");
	}
	return nullptr;
}
