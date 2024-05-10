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

#include <memory>

#include <utils/threadlocal_cache.h>

#include "threadlocal/gc.h"
#include "threadlocal/uid.h"
#include "threadlocal/allocator.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include "config.h"

#include "config/static_constants.h"

namespace scs {

class ThreadlocalContextStore
{
    struct context_t
    {
        MultitypeGarbageCollector<StorageDeltaClass>
            gc;

        UniqueIdentifier uid;

        context_t()
            : gc()
            , uid()
            {}
    };

    inline static utils::ThreadlocalCache<context_t, TLCACHE_SIZE> cache;

    inline static BlockAllocator<HashSetEntry> hash_allocator;

    ThreadlocalContextStore() = delete;

  public:

    template<typename delete_t>
    static void defer_delete(const delete_t* ptr)
    {
        (cache.get()).gc.template deferred_delete<delete_t>(ptr);
    }

    static HashSetEntry const& 
    get_hash(uint32_t idx)
    {
        return hash_allocator.get(idx);
    }

    static uint32_t 
    allocate_hash(HashSetEntry&& h)
    {
        return hash_allocator.allocate(std::move(h));
    }

    static uint64_t get_uid();

    static void
    post_block_clear()
    {
        auto& ctxs = cache.get_objects();
        for (auto& ctx : ctxs) {
            if (ctx) {
                ctx->gc.post_block_clear();
                // no need to reset uid, we're not going to overflow 2^48
                // rps cancelled through stop_rpcs()
            }
        }
        hash_allocator.reset();

        // no need to reset execution contexts -- those are automatically reset after every transaction
        // for some reason executing reset() on each threadlocal execution context
        // in the old post_block_clear caused a 7-8ms delay when it was a no-op (??? optimizer shenanigans, perhaps)
    }

    static void clear_entire_context()
    {
        cache.clear();
        hash_allocator.reset();
    }
};

} // namespace scs
