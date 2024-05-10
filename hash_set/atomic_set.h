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

#include <atomic>
#include <cstdint>
#include <vector>

#include "xdr/types.h"
#include "xdr/storage.h"

#include <utils/non_movable.h>

namespace scs {

class AtomicSet : public utils::NonMovableOrCopyable
{
    constexpr static float extra_buffer = 1.2;

    uint32_t capacity = 0;

    std::atomic<uint32_t>* array = nullptr;

    std::atomic<uint32_t> num_filled_slots = 0;

    constexpr static uint32_t TOMBSTONE = 0xFFFF'FFFF;

  public:
    AtomicSet(uint32_t max_capacity)
        : capacity(max_capacity * extra_buffer)
        , array(nullptr)
    {
        if (capacity > 0)
        {
           array = new std::atomic<uint32_t>[capacity] {};
        }
    }

    ~AtomicSet();

    void resize(uint32_t new_capacity);

    void clear();

    bool try_insert(const HashSetEntry& h);

    // throws if nexist
    void erase(const HashSetEntry& h);

    std::vector<HashSetEntry> get_hashes() const;
};

} // namespace scs
