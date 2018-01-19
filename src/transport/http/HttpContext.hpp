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
// File created on: 2017.03.28

// HttpContext.hpp


#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../TransportContext.hpp"
#include <memory>

class HttpContext: public TransportContext
{
protected:
	std::shared_ptr<HttpRequest> _request;
	std::shared_ptr<HttpResponse> _response;
	bool _http100ContinueSent;

public:
	HttpContext(const std::shared_ptr<Connection>& connection)
	: TransportContext(connection)
	, _http100ContinueSent(false)
	{};
	virtual ~HttpContext() = default;

	bool isHttp100ContinueSent() const
	{
		return _http100ContinueSent;
	}
	void setHttp100ContinueSent()
	{
		_http100ContinueSent = true;
	}

	std::shared_ptr<HttpRequest> getRequest()
	{
		return _request;
	}
	void setRequest(const std::shared_ptr<HttpRequest>& request)
	{
		_request = request;
	}
	void resetRequest()
	{
		_request.reset();
	}

	std::shared_ptr<HttpResponse> getResponse()
	{
		return _response;
	}
	void setResponse(const std::shared_ptr<HttpResponse>& response)
	{
		_response = response;
	}
	void resetResponse()
	{
		_response.reset();
	}
};
