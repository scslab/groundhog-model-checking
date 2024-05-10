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

#include "object/comparators.h"

#include "xdr/storage.h"

namespace scs
{

std::strong_ordering 
operator<=>(const HashSetEntry& d1, const HashSetEntry& d2)
{
	auto res = d1.index <=> d2.index;
	if (res != std::strong_ordering::equal)
	{
		return res;
	}

	return d1.hash <=> d2.hash;
}


} // namespace scs
