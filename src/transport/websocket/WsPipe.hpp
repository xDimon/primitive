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
// File created on: 2017.10.05

// WsPipe.hpp


#pragma once


#include "../Transport.hpp"
#include "../../net/TcpConnection.hpp"
#include "../../telemetry/Metric.hpp"

class WsPipe final : public Transport
{
private:
	std::shared_ptr<Handler> _handler;

public:
	WsPipe(const WsPipe&) = delete; // Copy-constructor
	WsPipe& operator=(WsPipe const&) = delete; // Copy-assignment
	WsPipe(WsPipe&&) noexcept = delete; // Move-constructor
	WsPipe& operator=(WsPipe&&) noexcept = delete; // Move-assignment

	WsPipe(
		const std::shared_ptr<Handler>& handler
	);

	~WsPipe() override;

	std::shared_ptr<Metric> metricRequestCount;
	std::shared_ptr<Metric> metricAvgRequestPerSec;
	std::shared_ptr<Metric> metricAvgExecutionTime;

	bool processing(const std::shared_ptr<Connection>& connection) override;

	void transmit(
		const std::shared_ptr<Connection>& connection,
		const char* data, size_t size,
		const std::string& contentType,
		bool close
	);
};
