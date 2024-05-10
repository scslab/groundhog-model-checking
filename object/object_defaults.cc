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

#include "object/object_defaults.h"

#include <cstring>

namespace scs
{

StorageObject
object_from_delta_class(StorageDeltaClass const& dc, std::optional<StorageObject> const& prev_object)
{
	StorageObject out;
	out.body.type(dc.type());

	if (prev_object)
	{
		if (prev_object -> body.type() != out.body.type())
		{
			throw std::runtime_error("type mismatch in object_from_delta_class");
		}
	}
	switch(out.body.type())
	{
		case ObjectType::RAW_MEMORY:
		{
			out.body.raw_memory_storage().data = dc.data();
			break;
		}
		case ObjectType::NONNEGATIVE_INT64:
		{
			out.body.nonnegative_int64() = dc.nonnegative_int64();
			break;
		}
		case ObjectType::HASH_SET:
		{
			out.body.hash_set().max_size = START_HASH_SET_SIZE;

			if (prev_object)
			{
				out.body.hash_set().max_size = prev_object -> body.hash_set().max_size;
				out.body.hash_set().hashes = prev_object -> body.hash_set().hashes;
			}
			break;
		}
		case ObjectType::KNOWN_SUPPLY_ASSET:
		{
			out.body.asset().amount = 0;
			if (prev_object)
			{
				out.body.asset().amount = prev_object -> body.asset().amount;
			}
			break;
		}
		break;
		default:
			throw std::runtime_error("unimpl sdc object_defaults.cc");
	}
	return out;
}

} /* scs */
