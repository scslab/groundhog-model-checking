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

%#include "xdr/types.h"

namespace scs
{

enum ObjectType
{
	RAW_MEMORY = 0,
	NONNEGATIVE_INT64 = 1,
	HASH_SET = 2,
	UNCONSTRAINED_INT64 = 3,
	KNOWN_SUPPLY_ASSET = 4,
};

const RAW_MEMORY_MAX_LEN = 4096;

struct RawMemoryStorage
{
	opaque data<RAW_MEMORY_MAX_LEN>;
};

struct HashSetEntry
{
	Hash hash;
	uint64 index;
};

struct HashSet
{
	HashSetEntry hashes<>;
	uint32 max_size;
};

const MAX_HASH_SET_SIZE = 65535;
const START_HASH_SET_SIZE = 64;

struct AssetObject {
	uint64 amount;
};

struct StorageObject
{
	union switch (ObjectType type)
	{
	case RAW_MEMORY:
		RawMemoryStorage raw_memory_storage;
	case NONNEGATIVE_INT64:
		int64 nonnegative_int64;
	case HASH_SET:
		HashSet hash_set;
	case UNCONSTRAINED_INT64:
		int64 unconstrained_int64;
	case KNOWN_SUPPLY_ASSET:
		AssetObject asset;
	} body;

	uint64 escrowed_fee;
};

// in case of none:
// first ensure all are of same type -- if not, all fail
// then apply the single type semantics, using default starting value

} /* scs */
