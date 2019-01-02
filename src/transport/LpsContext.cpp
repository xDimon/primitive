// Copyright © 2018-2019 Dmitriy Khaustov
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
#include "../compression/CompressorFactory.hpp"

LpsContext::LpsContext(
	const std::shared_ptr<ServicePart>& servicePart,
	const std::shared_ptr<TransportContext>& context,
	bool aggregation,
	bool compression
)
: _service(servicePart)
, _context(context)
, _aggregation(aggregation)
, _compression(compression)
, _close(false)
, _closed(false)
{
	if (dynamic_cast<WsContext*>(_context.get()))
	{
	}
	else if (dynamic_cast<HttpContext*>(_context.get()))
	{
	}
	else
	{
		throw std::runtime_error("Unsupported transport context");
	}
}

LpsContext::~LpsContext()
{
	if (!_closed)
	{
		try { send(); } catch (...) {}
	}
}

void LpsContext::assignSession(const std::shared_ptr<Session>& session)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return wsContext->assignSession(session);
	}
	_session = session;
}

std::shared_ptr<Session> LpsContext::getSession()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return wsContext->getSession();
	}
	return _session.lock();
}

void LpsContext::resetSession()
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	auto wsContext = std::dynamic_pointer_cast<WsContext>(_context);
	if (wsContext)
	{
		return std::dynamic_pointer_cast<WsContext>(_context)->resetSession();
	}
	_session.reset();
}

SVal LpsContext::recv()
{
	static Log log_("_lpsContext");
	auto service = _service.lock();
	Log& log = service ? service->log() : log_;

	std::string in;

	char context_ = '?';
	if (auto wsContext = std::dynamic_pointer_cast<WsContext>(_context))
	{
		context_ = 'W';
		in.assign(wsContext->getFrame()->dataPtr(), wsContext->getFrame()->dataLen());
	}
	else if (auto httpContext = std::dynamic_pointer_cast<HttpContext>(_context))
	{
		context_ = 'H';
		in.assign(httpContext->getRequest()->dataPtr(), httpContext->getRequest()->dataLen());
	}

	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	char compress_ = '?';
	if (_compression)
	{
		if (in[0] == 0 || in[0] == 1)
		{
			compress_ = in[0] ? 'C' : 'N';

			std::vector<char> out1(in.begin(), in.end());
			std::vector<char> out2;

			CompressorFactory::create("gzip")->inflate(out1, out2);

			in.assign(out2.begin(), out2.end());
		}
		else
		{
			compress_ = 'E';
			_compression = false;
		}
	}
	else
	{
		compress_ = 'R';
	}

	static const size_t maxLength = 2048;
	if (in.size() > maxLength)
	{
		std::string reduced;
		static const std::string replacer("<...rejected...>");

		std::copy(in.begin(), in.begin() + maxLength / 2 - replacer.size() / 2, std::inserter(reduced, reduced.end()));
		std::copy(replacer.begin(), replacer.end(), std::inserter(reduced, reduced.end()));
		std::copy(in.end() - maxLength / 2 + replacer.size() / 2, in.end(), std::inserter(reduced, reduced.end()));

		log.info("%c%c%s << %s", context_, compress_, tag().c_str(), reduced.c_str());
	}
	else
	{
		log.info("%c%c%s << %s", context_, compress_, tag().c_str(), in.c_str());
	}

	auto result = SerializerFactory::create("json", Serializer::STRICT)->decode(in);

	return result;
}

void LpsContext::out(SVal value, bool close)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (_closed)
	{
		return;
	}

	_output.push(std::move(value));
	if (close)
	{
		_close = true;
	}

	if (!_aggregation && std::dynamic_pointer_cast<WsContext>(_context))
	{
		try { send(); } catch (...) {}
	}
	else
	{
		if (!_timeout)
		{
			_timeout = std::make_shared<Timeout>(
				[wp = std::weak_ptr<LpsContext>(std::dynamic_pointer_cast<LpsContext>(ptr()))]
				{
					auto iam = wp.lock();
					if (iam) try { iam->send(); } catch (...) {}
				},
				"Timeout to end long polling"
			);
		}

		if (dynamic_cast<WsContext*>(_context.get()))
		{
			_timeout->restart(std::chrono::milliseconds(50));
		}
		else
		{
			_timeout->restart(std::chrono::milliseconds(500));
		}
	}
}

void LpsContext::send()
{
	static Log log_("_lpsContext");
	auto service = _service.lock();
	Log& log = service ? service->log() : log_;

	std::lock_guard<std::recursive_mutex> lockGuard(_mutex);

	if (_closed)
	{
		return;
	}

	char context_ = '?';
	char compress_ = '?';
	std::string out;

	if (dynamic_cast<WsContext*>(_context.get()))
	{
		context_ = 'W';

		_closed = _closed || _close;

		if (_output.size() == 1)
		{
			auto output = std::move(_output.front());
			_output.pop();

			out = SerializerFactory::create("json")->encode(std::move(output));

			if (_compression)
			{
				std::vector<char> out1(out.begin(), out.end());
				std::vector<char> out2;

				CompressorFactory::create("gzip")->deflate(out1, out2);

				compress_ = out2[0] ? 'C' : 'N';
				_context->transmit(out2, "bin", _close);
			}
			else
			{
				compress_ = 'R';
				_context->transmit(out, "json", _close);
			}
		}
		else if (!_output.empty())
		{
			SArr output;

			while (!_output.empty())
			{
				output.emplace_back(std::move(_output.front()));
				_output.pop();
			}

			out = SerializerFactory::create("json")->encode(std::move(output));

			if (_compression)
			{
				std::vector<char> out1(out.begin(), out.end());
				std::vector<char> out2;

				CompressorFactory::create("gzip")->deflate(out1, out2);

				compress_ = out2[0] ? 'C' : 'N';
				_context->transmit(out2, "bin", _close);
			}
			else
			{
				compress_ = 'R';
				_context->transmit(out, "json", _close);
			}
		}
		else
		{
			return;
		}
	}
	else if (dynamic_cast<HttpContext*>(_context.get()))
	{
		context_ = 'H';

		_closed = true;

		SArr output;

		while (!_output.empty())
		{
			output.emplace_back(std::move(_output.front()));
			_output.pop();
		}

		out = SerializerFactory::create("json")->encode(output);

		if (_compression)
		{
			std::vector<char> out1(out.begin(), out.end());
			std::vector<char> out2;

			CompressorFactory::create("gzip")->deflate(out1, out2);

			compress_ = out[0] ? 'C' : 'N';
			_context->transmit(out2, "application/octet-stream", true);
		}
		else
		{
			compress_ = 'R';
			_context->transmit(out, "application/json; charset=utf-8", true);
		}
	}

	static const size_t maxLength = 2048;
	if (out.size() > maxLength)
	{
		static const std::string replacer("<...rejected...>");
		std::copy(replacer.begin(), replacer.end(), out.begin() + maxLength / 2 - replacer.size() / 2);
		std::copy(out.end() - maxLength / 2 + replacer.size() / 2, out.end(), out.begin() + maxLength / 2 + replacer.size() / 2);
		out.resize(maxLength);
	}

	log.info("%c%c%s >> %s", context_, compress_, tag().c_str(), out.c_str());
}
