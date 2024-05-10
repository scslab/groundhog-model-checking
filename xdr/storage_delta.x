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
%#include "xdr/storage.h"

namespace scs
{

enum DeltaType
{
	DELETE_LAST = 0,
	RAW_MEMORY_WRITE = 1,
	NONNEGATIVE_INT64_SET_ADD = 2,
	HASH_SET_INSERT = 3,
	HASH_SET_INCREASE_LIMIT = 4,
	HASH_SET_CLEAR = 5,
	UNCONSTRAINED_INT64_SET_ADD = 6,
	UNCONSTRAINED_INT64_SET_MAX = 7,
	UNCONSTRAINED_INT64_SET_XOR = 8,
	ASSET_OBJECT_ADD = 9,
};

% static_assert(static_cast<uint64_t>(-1) == 0xFFFF'FFFF'FFFF'FFFF, "requires two's complement");

struct set_add_t
{
	int64 set_value;
	int64 delta;
};

struct set_xor_t
{
	int64 set_value;
	uint64 bitmap;
};

struct set_max_t
{
	int64 set_value;
	int64_t max;
};

union StorageDelta switch (DeltaType type)
{
	case DELETE_LAST:
		void;
	case RAW_MEMORY_WRITE:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64_SET_ADD:
		set_add_t set_add_nonnegative_int64;
	case HASH_SET_INSERT:
		HashSetEntry hash;
	case HASH_SET_INCREASE_LIMIT:
		// enforced to be at most uint16_max
		// (MAX_HASH_SET_SIZE)
		uint32 limit_increase;
	case HASH_SET_CLEAR:
		uint64 threshold;
	case UNCONSTRAINED_INT64_SET_ADD:
		set_add_t set_add_unconstrained_int64;
	case UNCONSTRAINED_INT64_SET_MAX:
		set_max_t set_max_unconstrained_int64;
	case UNCONSTRAINED_INT64_SET_XOR:
		set_xor_t set_xor_unconstrained_int64;
	case ASSET_OBJECT_ADD:
		int64 asset_delta;
};

// REFUND DESIGN:
// imo this is the simplest
// alternative is tracking excesses per block
// into "refundable events" and then paying these back
// afterwards, but that seems like a lot of extra work.
// 
// on each StorageObject, track "invested/locked units",
// then give a "claim_all_extra" action that users can do
// (once per block, per addrkey) to reclaim any extra

// All StorageDeltas on one object must all
// modify the same ObjectType
// and must agree on the information contained within 
// a StorageDeltaClass
// TODO: A more careful equivalence relation
// on this class would allow things like hash set limit set,
// not just add().

struct UnconstrainedInt64DeltaClass
{
	int64 set_value;
	DeltaType delta;
};

union StorageDeltaClass switch (ObjectType type)
{
	case RAW_MEMORY:
		opaque data<RAW_MEMORY_MAX_LEN>;
	case NONNEGATIVE_INT64:
		int64 nonnegative_int64;
	case HASH_SET:
		void;
	case UNCONSTRAINED_INT64:
		UnconstrainedInt64DeltaClass unconstrained_int64_modtype;
	case KNOWN_SUPPLY_ASSET:
		void;
};


struct IndexedModification
{
    AddressAndKey addr;
    Hash tx_hash;
    StorageDelta delta;
};

typedef IndexedModification ModIndexLog<>;


/*

struct DeltaPriority
{
	// higher custom_priority beats lower (empty) custom_priority
	uint32 custom_priority;

	//higher gas bid wins ties
	uint64 gas_rate_bid;

	Hash tx_hash;

	// the ith delta created by a tx gets id i.  Final uniqueness tiebreaker.
	uint32 delta_id_number;
};

enum TypeclassValence
{
	TV_FREE = 0,
	TV_RAW_MEMORY_WRITE = 1,
	TV_NONNEGATIVE_INT64_SET = 2,
	TV_ERROR = 3,
};

struct DeltaValence
{
	union switch(TypeclassValence type)
	{
		case TV_FREE:
			void;
		case TV_RAW_MEMORY_WRITE:
			opaque data<RAW_MEMORY_MAX_LEN>;
		case TV_NONNEGATIVE_INT64_SET:
			int64 set_value;
		case TV_ERROR:
			void;
	} tv;

	uint32 deleted_last; // 0 = false, nonzero = true;
};

*/

} /* scs */
