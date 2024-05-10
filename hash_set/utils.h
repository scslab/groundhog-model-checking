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

#include <algorithm>
#include "xdr/storage.h"
#include "object/comparators.h"

namespace scs
{

[[maybe_unused]]
static void normalize_hashset(HashSet& hs)
{
	auto& h_list = hs.hashes;
	std::sort(h_list.begin(), h_list.end(), std::greater<HashSetEntry>());
}

[[maybe_unused]]
static void clear_hashset(HashSet& hs, uint64_t threshold)
{
	auto& h_list = hs.hashes;
    uint32_t min = 0, max = h_list.size();
    uint32_t med = 0;

    while(true)
    {
        med = (max + min) / 2;
        if (max == min)
        {
            break;
        }

        if (h_list[med].index > threshold)
        {
            min = med + 1;
        } 
        else 
        {
            max = med;
        }
    }
    h_list.resize(med);

/*
	for (auto it = h_list.begin(); it != h_list.end();)
    {
        if (it -> index <= threshold)
        {
            it = h_list.erase(it);
        } else
        {
            ++it;
        }
    } */
}

}