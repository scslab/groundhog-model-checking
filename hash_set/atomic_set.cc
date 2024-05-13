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

#include "hash_set/atomic_set.h"

#include "crypto/crypto_utils.h"

#include "threadlocal/threadlocal_context.h"

using xdr::operator==;

namespace scs {

AtomicSet::~AtomicSet()
{
    if (array != nullptr) {
        delete[] array;
    }
}

void
AtomicSet::resize(uint32_t new_capacity)
{
    uint32_t new_alloc_size = new_capacity * extra_buffer;
    if (new_alloc_size < capacity) {
        return;
    }

    if (array != nullptr) {
        delete[] array;
    }
    capacity = new_alloc_size;
    array = new std::atomic<uint32_t>[capacity] {};

    num_filled_slots = 0;
}

void
AtomicSet::clear()
{
    for (auto i = 0u; i < capacity; i++) {
        array[i] = 0;
    }
    num_filled_slots = 0;
}

bool
AtomicSet::try_insert(const HashSetEntry& h, uint32_t start_idx)
{
    if (start_idx == UINT32_MAX) {
        start_idx
            = shorthash(h.hash.data(), h.hash.size(), capacity);
    }
    uint32_t idx = start_idx;

    uint32_t alloc = ThreadlocalContextStore::allocate_hash(HashSetEntry(h));

    // not an interesting place to yield
    const uint32_t cur_filled_slots
        = num_filled_slots.load(std::memory_order_relaxed);

    if (cur_filled_slots >= capacity) {
        return false;
    }

    do {
        while (true) {
            conditional_yield();
            uint32_t local = array[idx].load(std::memory_order_relaxed);

            // insert can be allowed to overwrite tombstones
            if (local == 0) {
                conditional_yield();
                if (array[idx].compare_exchange_strong(
                        local, alloc, std::memory_order_relaxed)) {
                    conditional_yield();

                    num_filled_slots.fetch_add(1, std::memory_order_relaxed);

                    return true;
                } else {
                    continue;
                }
            }

            if (local != TOMBSTONE) {
                if (ThreadlocalContextStore::get_hash(local) == h) {
                    return false;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        idx++;
        if (idx == capacity) {
            idx = 0;
        }

    } while (idx != start_idx);
    return false;
}

void
AtomicSet::erase(const HashSetEntry& h, uint32_t start_idx)
{
    if (start_idx == UINT32_MAX) {
        start_idx
            = shorthash(h.hash.data(), h.hash.size(), capacity);
    }
    uint32_t idx = start_idx;

    do {
        while (true) {
            conditional_yield();
            uint32_t local = array[idx].load(std::memory_order_relaxed);

            if (local != 0 && local != TOMBSTONE) {
                if (ThreadlocalContextStore::get_hash(local) == h) {
                    conditional_yield();
                    if (array[idx].compare_exchange_strong(
                            local, TOMBSTONE, std::memory_order_relaxed)) {
                        conditional_yield();
                        num_filled_slots.fetch_sub(1, std::memory_order_relaxed);
                        return;
                    } else {
                        continue;
                    }
                }
            }

            if (local == 0) {
                throw std::runtime_error("deletion failed to find elt");
            }
            break;
        }

        idx++;
        if (idx == capacity) {
            idx = 0;
        }

    } while (idx != start_idx);

    throw std::runtime_error("deletion failed after complete scan");
}

std::vector<HashSetEntry>
AtomicSet::get_hashes() const
{
    std::vector<HashSetEntry> out;

    for (uint32_t i = 0; i < capacity; i++) {
        uint32_t idx = array[i].load(std::memory_order_relaxed);
        if (idx != 0 && idx != TOMBSTONE) {
            out.push_back(ThreadlocalContextStore::get_hash(idx));
        }
    }
    return out;
}

} // namespace scs
