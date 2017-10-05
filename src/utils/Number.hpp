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
// File created on: 2017.07.06

// Number.hpp


#pragma once


namespace Number
{
	template<typename D, typename S>
	void assign(D& dst, const S& src, const D& def = D())
	{
		if (
			src > std::numeric_limits<D>::max() ||
			src < std::numeric_limits<D>::min()
		)
		{
			dst = def;
		}
		else
		{
			dst = static_cast<D>(src);
		}
	}
};
