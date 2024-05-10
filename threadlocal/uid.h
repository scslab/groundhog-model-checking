#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utils/threadlocal_cache.h>

#include <cstdint>
#include "config/static_constants.h"

namespace scs
{

/**
 * Outputs a 48-bit unique ID
 * at every call to get().
 * 
 * Output is <48 bit tag><16 bit 0s>
 */ 
class UniqueIdentifier
{
	constexpr static uint64_t inc = ((uint64_t)1) << 16;
	uint64_t id;

	static_assert(TLCACHE_SIZE <= 512, "insufficient bit length in id offset initialization");

public:

	UniqueIdentifier()
		: id(static_cast<uint64_t>(utils::ThreadlocalIdentifier::get()) << 55)
		{}

	// never returns 0
	uint64_t get()
	{
		return id += inc;
	}
};

}