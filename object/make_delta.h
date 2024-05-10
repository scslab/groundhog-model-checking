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

#include "xdr/types.h"

#include <xdrpp/types.h>

#include "xdr/storage_delta.h"

namespace scs {

StorageDelta
make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data);

StorageDelta
make_delete_last();

StorageDelta
make_nonnegative_int64_set_add(int64_t set, int64_t add);

StorageDelta
make_hash_set_insert(Hash const& h, uint64_t threshold);

StorageDelta
make_hash_set_insert(HashSetEntry const& entry);

StorageDelta
make_hash_set_increase_limit(uint16_t limit);

StorageDelta
make_hash_set_clear(uint64_t threshold);

StorageDelta
make_asset_add(int64_t delta);

} // namespace scs
