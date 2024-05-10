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

#include <cstdint>

namespace scs
{

// must be greater than # of threads created in one run
constexpr static uint32_t TLCACHE_SIZE = 512;
constexpr static uint32_t TLCACHE_BITS_REQUIRED = 9; // 512 = 2^9

static_assert (static_cast<uint32_t>(1) << TLCACHE_BITS_REQUIRED == TLCACHE_SIZE, "mismatch");

}
