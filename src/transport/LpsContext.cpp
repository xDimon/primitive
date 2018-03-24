// Copyright © 2018 Dmitriy Khaustov
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
// File created on: 2018.01.23

// LpsContext.cpp


#include "LpsContext.hpp"
#include "http/HttpContext.hpp"
#include "websocket/WsContext.hpp"

LpsContext::LpsContext(const std::shared_ptr<ServicePart>& servicePart, const std::shared_ptr<TransportContext>& context, const std::string& data)
: _log("LpsContext")
, _context(context)
{
	if (data.empty())
	{
		return;
	}
	else if (std::dynamic_pointer_cast<WsContext>(_context))
	{
		_log.info("WS-IN  %s", data.c_str());
	}
	else if (std::dynamic_pointer_cast<HttpContext>(_context))
	{
		_log.info("HTTP-IN  %s", data.c_str());
	}
	else
	{
		_log.info("?\?\?-IN  %s", data.c_str());
	}
}

void LpsContext::assignSession(const std::shared_ptr<Session>& session)
{
	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return std::dynamic_pointer_cast<WsContext>(_context)->assignSession(session);
	}
	_session = session;
}

std::shared_ptr<Session> LpsContext::getSession()
{
	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return std::dynamic_pointer_cast<WsContext>(_context)->getSession();
	}
	return _session.lock();
}

void LpsContext::resetSession()
{
	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return std::dynamic_pointer_cast<WsContext>(_context)->resetSession();
	}
	_session.reset();
}

void LpsContext::out(const SVal* value, bool close)
{
	_output.push(value);

	if (std::dynamic_pointer_cast<WsContext>(_context))
	{
		if (close)
		{
			auto session = getSession();
			if (session)
			{
				session->assignContext(nullptr);
			}
		}
		send(close);
	}
	else // if (close || !!std::dynamic_pointer_cast<WsContext>(_context))
	{
		if (!_timeout)
		{
			_timeout = std::make_shared<Timeout>(
				[wp = std::weak_ptr<LpsContext>(std::dynamic_pointer_cast<LpsContext>(ptr())), close]
				{
					auto iam = wp.lock();
					if (iam)
					{
						if (close)
						{
							auto session = iam->getSession();
							if (session)
							{
								session->assignContext(nullptr);
							}
						}
						iam->send(close);
					}
				}
			);
		}
		_timeout->restart(std::chrono::milliseconds(100));
	}
}

void LpsContext::send(bool disconnect)
{
	if (std::dynamic_pointer_cast<WsContext>(_context))
	{
		while (!_output.empty())
		{
			auto element = const_cast<SVal*>(_output.front());
			_output.pop();

			auto out = SerializerFactory::create("json")->encode(element);

			_context->transmit(out, "text", disconnect && _output.empty());

			_log.info("WS-OUT %s", out.c_str());
		}
	}
	else if (std::dynamic_pointer_cast<HttpContext>(_context))
	{
		auto output = std::make_unique<SArr>();

		while (!_output.empty())
		{
			auto element = const_cast<SVal*>(_output.front());
			_output.pop();
			output->insert(element);
		}

		auto out = SerializerFactory::create("json")->encode(output.get());

		_context->transmit(out, "application/json; charset=utf-8", disconnect);

		_log.info("HTTP-OUT %s", out.c_str());
	}
	else
	{
		_log.info("?\?\?-OUT ?\?\?");
		throw std::runtime_error("Internal error: unsupported transport context");
	}
}
