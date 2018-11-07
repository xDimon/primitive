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
// File created on: 2017.08.17

// ClientPart.cpp


#include "ClientPart.hpp"
#include "../../src/transport/http/HttpRequestExecutor.hpp"
#include "Service.hpp"
#include "../../src/transport/Transports.hpp"
#include "../../src/telemetry/SysInfo.hpp"
#include "../../src/utils/Time.hpp"
#include "../../src/storage/DbManager.hpp"
#include "../../src/transport/http/HttpContext.hpp"
#include "../../src/telemetry/TelemetryManager.hpp"
#include <iomanip>

status::ClientPart::ClientPart(const std::shared_ptr<::Service>& service)
: ServicePart(std::dynamic_pointer_cast<status::Service>(service))
{
	_name = "client";
	_log.setName(service->name() + ":" + _name);
	_log.setDetail(Log::Detail::TRACE);
}

void status::ClientPart::init(const Setting& setting)
{
	try
	{
		std::string transportName;
		if (!setting.lookupValue("transport", transportName) || transportName.empty())
		{
			throw std::runtime_error("Field transport undefined or empty");
		}

		std::string uri;
		if (!setting.lookupValue("uri", uri) || uri.empty())
		{
			throw std::runtime_error("Field uri undefined or empty");
		}

		auto transport = Transports::get(transportName);
		if (!transport)
		{
			throw std::runtime_error(std::string("Transport '") + transportName + "' not found");
		}

		try
		{
			transport->bindHandler(
				uri,
				std::make_shared<ServerTransport::Handler>(
					[wp = std::weak_ptr<ClientPart>(std::dynamic_pointer_cast<ClientPart>(ptr()))]
					(const std::shared_ptr<Context>& context)
					{
						auto iam = wp.lock();
						if (iam)
						{
							iam->handle(context);
						}
					}
				)
			);
		}
		catch (const std::exception& exception)
		{
			throw std::runtime_error("Can't bind uri '" + uri + "' on transport '" + transportName + "': " + exception.what());
		}
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error("Fail init part '" + _name + "' ← " + exception.what());
	}
}

void status::ClientPart::handle(const std::shared_ptr<Context>& context)
{
	// Приводим тип контекста
	auto httpContext = std::dynamic_pointer_cast<HttpContext>(context);
	if (!httpContext)
	{
		throw std::runtime_error("Bad context-type for this service");
	}

	SObj input;
	try
	{
		_log.info("IN  %s", httpContext->getRequest()->uri().query().c_str());

//		auto&& rawInput = SerializerFactory::create("uri")->decode(
//			httpContext->getRequest()->uri().query()
//		);
//
//		input = rawInput.is<SObj>() ? rawInput.as<SObj>() : SObj();

		std::ostringstream oss;

		auto tsStart = std::chrono::system_clock::to_time_t(Time::startTime);
		tm tmStart{};
		::localtime_r(&tsStart, &tmStart);

		auto tsNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		tm tmNow{};
		::localtime_r(&tsNow, &tmNow);

		auto run = tsNow - tsStart;

		oss << "=============================================\n"
			<< "GENERAL\n"
			<< "\n";

		char buff1[40]; std::strftime(buff1, sizeof(buff1), "%Y-%m-%d %X %Z", &tmNow);
		char buff2[40]; std::strftime(buff2, sizeof(buff2), "%Y-%m-%d %X", &tmStart);

		oss << "Time now:                   " << buff1 << "\n"
			<< "Run since:                  " << buff2 << "\n"
//			<< "Time now:                   " << std::put_time(&tmNow, "%Y-%m-%d %X %Z") << "\n"
//			<< "Run since:                  " << std::put_time(&tmStart, "%Y-%m-%d %X") << "\n"
			<< "Running:                    "
											<< std::setw(9) << std::setfill(' ') << (run/86400) << "d "
											<< std::setw(2) << std::setfill('0') << (run/3600%24) << ":"
											<< std::setw(2) << std::setfill('0') << (run/60%60) << ":"
											<< std::setw(2) << std::setfill('0') << (run%60) << "\n"
			<< "PID:                           " << std::setw(7) << std::setfill(' ') << getpid() << "\n"
			<< "\n";

		oss << "=============================================\n"
			<< "RESOURCES\n"
			<< "\n";
		if (SysInfo::getInstance()._run)
		{
			oss
			<< "CPU current usage:             " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3)
			<< SysInfo::getInstance()._cpuUsageOnPercent->avgPerSec(std::chrono::seconds(15)) << " %\n"
			<< "Physical memory current usage: " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0)
			<< SysInfo::getInstance()._memoryUsage->avg(std::chrono::seconds(15)) << " kB\n"
			<< "\n"
			<< "User CPU time used:            " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3)
			<< SysInfo::getInstance()._cpuUsageByUserOnTime->sum(1) << " s\n"
			<< "System CPU time used:          " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3)
			<< SysInfo::getInstance()._cpuUsageBySystemOnTime->sum(1) << " s\n"
			<< "Maximum resident set size:     " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0)
			<< SysInfo::getInstance()._memoryMaxUsage->sum(1) << " kB\n"
			<< "Soft/hard page faults:         " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0)
			<< SysInfo::getInstance()._pageSoftFaults->sum(1) << "/" << SysInfo::getInstance()._pageHardFaults->avg(1) << "\n"
			<< "Block input/output operations: " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0)
			<< SysInfo::getInstance()._blockInputOperations->sum(1) << "/" << SysInfo::getInstance()._blockOutputOperations->avg(1)
			<< "\n"
			<< "Vol./Invol. context switches:  " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0)
			<< SysInfo::getInstance()._voluntaryContextSwitches->sum(1) << "/"
			<< SysInfo::getInstance()._involuntaryContextSwitches->avg(1) << "\n"
			<< "\n";
		}
		else
		{
			oss
			<< "SysInfo wasn't run...\n\n";
		}

		oss << "=============================================\n"
			<< "DATABASE\n"
			<< "\n";

		DbManager::forEach([&oss](const std::shared_ptr<DbConnectionPool>& pool){
			auto queryCount = pool->metricAvgQueryPerSec->sum(std::chrono::seconds(15));
			oss
			<< "---------------------------------------------\n"
			<< "[" << pool->name() << "]\n"
			<< "All connections:               " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << pool->metricSumConnections->sum(1) << "\n"
			<< "Successful queries:            " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << pool->metricSuccessQueryCount->sum(1) << "\n"
			<< "Fail queries:                  " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << pool->metricFailQueryCount->sum(1) << "\n"
			<< "Current avg execution speed:   " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3) << (queryCount/15) << " qps\n"
			<< "Current avg execution time:    " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3) << (queryCount>0 ? (pool->metricAvgExecutionTime->sum(std::chrono::seconds(15))/queryCount)*1000 : 0) << " ms\n"
			<< "\n";
		});

		oss << "=============================================\n"
			<< "TRANSPORT\n"
			<< "\n";

		Transports::forEach([&oss](const std::shared_ptr<ServerTransport>& transport){
			auto requestCount = transport->metricAvgRequestPerSec->sum(std::chrono::seconds(15));
			oss
			<< "---------------------------------------------\n"
			<< "[" << transport->name() << "]\n"
			<< "All connections:               " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << transport->metricConnectCount->sum(1) << "\n"
			<< "Request received :             " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(0) << transport->metricRequestCount->sum(1) << "\n"
			<< "Current avg execution speed:   " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3) << (requestCount/15) << " rps\n"
			<< "Current avg execution time:    " << std::setw(7) << std::setfill(' ') << std::fixed << std::setprecision(3) << (requestCount>0 ? (transport->metricAvgExecutionTime->sum(std::chrono::seconds(15))/requestCount)*1000 : 0) << " ms\n"
			<< "\n";
		});

		if (input.has("raw"))
		{
			oss << "=============================================\n"
				<< "RAW METRICS\n"
				<< "\n";

			oss
				<< std::setw(51) << std::left << std::setfill(' ') << "NAME"
				<< std::setw(11) << std::right << std::setfill(' ') << "Sum"
				<< std::setw(11) << std::right << std::setfill(' ') << "Sum 15s"
				<< std::setw(11) << std::right << std::setfill(' ') << "Average"
				<< std::setw(11) << std::right << std::setfill(' ') << "Avg 15s"
				<< std::setw(11) << std::right << std::setfill(' ') << "Avg 1/sec"
				<< "\n";

			for (auto i : TelemetryManager::metrics())
			{
				oss
					<< std::setw(50) << std::left << std::setfill(' ') << i.first << " "
					<< std::setw(10) << std::right << std::setfill(' ') << std::fixed << i.second->sum(1) << " "
					<< std::setw(10) << std::right << std::setfill(' ') << std::fixed << i.second->sum(std::chrono::seconds(15)) << " "
					<< std::setw(10) << std::right << std::setfill(' ') << std::fixed << i.second->avg(1) << " "
					<< std::setw(10) << std::right << std::setfill(' ') << std::fixed << i.second->avg(std::chrono::seconds(15)) << " "
					<< std::setw(10) << std::right << std::setfill(' ') << std::fixed << i.second->avgPerSec(std::chrono::seconds(15))
					<< "\n";
			}
		}

      	oss << "=============================================\n";

		httpContext->transmit(std::move(oss.str()), "text/pain; charset=utf-8", true);

		_log.info("OUT Send status info (load=%0.03f%%)", SysInfo::getInstance()._cpuUsageOnPercent->avgPerSec(std::chrono::seconds(15)));
	}
	catch (const std::exception& exception)
	{
		auto serializer = SerializerFactory::create("json");

		SObj output;
		output.emplace("status", false);
		output.emplace("message", exception.what());
		std::string out(serializer->encode(output));

		httpContext->transmit(out, "text/plain; charset=utf-8", true);

		_log.info("OUT %s", out.c_str());

		return;
	}
}
