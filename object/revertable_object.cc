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

#include "object/revertable_object.h"

#include "threadlocal/threadlocal_context.h"

#include "object/object_defaults.h"
#include "object/comparators.h"

#include "hash_set/utils.h"

#include <utils/assert.h>
#include <utils/compat.h>

using xdr::operator==;

namespace scs {

/**
 * Algorithm: try_set/revert sets a uint64 to be <unique tag, 48 bits> <active
 *count, 8 bits> <7 empty><is_finalized flag, 1 bit> then sets pointer (separate
 *word).
 *
 * Specifically:
 *
 * 	every modification does the following:
 * 		read tag, then pointer atomically (separately)
 * 			if tag.active_count > 0 but pointer is nullptr, or
 *              tag.active_count == 0 but pointer is not nullpointer, then we
 *              had a shorn read -- retry compute next tag, pointer
 *              compare_exchange on tag, then write pointer
 *
 * 			    uses release_acquire ordering
 *
 * TODO: The extra flags could be used for DELETE_FIRST, but ultimately
 * without a way to prioritize the DELETE_FIRST over other writes,
 * DELETE_FIRST doesn't accomplish all that much (over e.g. instead a 2phase
 * declare_close + close paradigm).
 * That said, DELETE_FIRST doesn't really make sense for the use case I'd been
 * thinking about (closing an account) -- what I really need is a lock,
 * and moreover I need some kind of precedence for some writers over others
 * What I really want is a 2phase account close setup, I think.
 *
 * The unique tag ids ensure that there's a single total order of modifications
 *on the tag. Release-acquire ordering means that one thread might read none of
 *the writes of another thread, the first write, or the first + second (but not
 *the second without the first).
 *
 * The pointer value only changes to/from nullptr (never from one valid pointer
 *to another).
 *
 * If a thread reads "active_count==1" and "non nullptr" in a revert() call,
 * then it writes "active_count=0" and "nullptr".
 *
 * Whichever op goes next must read "active_count=0", and will retry until it
 *sees the nullptr.
 *
 * Because the pointer is read with acquire, and then the tag is written with
 *release, any subsequent op cannot read the earlier nonnull value of the ptr
 *(that was overwritten to nullptr).
 *
 **/

uint64_t
swap_id(uint64_t old_tag, uint64_t new_id)
{
    return new_id + (old_tag & 0xFFFF);
    // return input + (static_cast<uint64_t>(1) << 16);
}

uint64_t
add_inflight(uint64_t input)
{
    return input + (static_cast<uint64_t>(1) << 8);
}

uint64_t
remove_inflight(uint64_t input)
{
    return input - (static_cast<uint64_t>(1) << 8);
}

bool
has_inflight(uint64_t input)
{
    return (input & 0xFF00);
}

bool
is_finalized(uint64_t input)
{
    return input & 1;
}

bool
expect_nonnull(uint64_t input)
{
    return (input & 0xFFFF);
}

void
RevertableBaseObject::Rewind::commit()
{
    if (do_revert) {
        obj->commit();
    }
    do_revert = false;
}

RevertableBaseObject::Rewind::~Rewind()
{
    if (do_revert) {
        obj->revert();
    }
    do_revert = false;
}

void
RevertableBaseObject::clear_required_type()
{
    required_type = std::nullopt;
}

std::optional<RevertableBaseObject::Rewind>
RevertableBaseObject::try_set(StorageDeltaClass const& new_obj)
{
    if (required_type) {
        if (new_obj.type() != *required_type) {
            return std::nullopt;
        }
    }

    uint64_t new_id = ThreadlocalContextStore::get_uid();
    while (true) {
        const uint64_t t = tag.load(std::memory_order_acquire);
        StorageDeltaClass* current = obj.load(std::memory_order_acquire);
        uint64_t expect = t;

        if (expect_nonnull(t) && (current == nullptr)) {
            // shorn read
            continue;
        }
        if ((!expect_nonnull(t)) && (current != nullptr)) {
            // shorn read
            continue;
        }

        if (current == nullptr) {
            StorageDeltaClass* new_candidate = new StorageDeltaClass(new_obj);

            uint64_t new_tag = swap_id(t, new_id);
            new_tag = add_inflight(new_tag);

            if (tag.compare_exchange_weak(
                    expect, new_tag, std::memory_order_release)) {
                obj.store(new_candidate, std::memory_order_release);
                return { Rewind(*this, true) };
            }
            delete new_candidate;
            continue;
        }
        // current != nullptr

        bool matches = ((*current) == new_obj);

        if (!matches) {
            return std::nullopt;
        }

        // matches is true

        if (is_finalized(t)) {
            return { Rewind(*this, false) };
        }

        uint64_t new_tag = swap_id(t, new_id);
        new_tag = add_inflight(new_tag);

        if (tag.compare_exchange_weak(
                expect, new_tag, std::memory_order_release)) {
            return { Rewind(*this, true) };
        }
    }
}

void
RevertableBaseObject::commit()
{
    tag.fetch_or(1, std::memory_order_acq_rel);
}

void
RevertableBaseObject::revert()
{
    uint64_t new_id = ThreadlocalContextStore::get_uid();
    while (true) {
        const uint64_t t = tag.load(std::memory_order_acquire);
        auto* current = obj.load(std::memory_order_acquire);
        uint64_t expect = t;

        if (expect_nonnull(t) && (current == nullptr)) {
            // shorn read
            continue;
        }
        if ((!expect_nonnull(t)) && (current != nullptr)) {
            // shorn read
            continue;
        }
        if (is_finalized(t)) {
            return;
        }

        uint64_t new_tag = swap_id(t, new_id);
        new_tag = remove_inflight(new_tag);

        if (tag.compare_exchange_weak(
                expect, new_tag, std::memory_order_release)) {

            if (!has_inflight(new_tag)) {
                obj.store(nullptr, std::memory_order_release);
                ThreadlocalContextStore::defer_delete(current);
            }
            return;
        }
    }
}

RevertableBaseObject::RevertableBaseObject()
    : tag(0)
    , obj(nullptr)
    , required_type(std::nullopt)
{}

RevertableBaseObject::RevertableBaseObject(const StorageObject& obj)
    : tag(0)
    , obj(nullptr)
    , required_type(obj.body.type())
{}

RevertableBaseObject::~RevertableBaseObject()
{
    auto* ptr = obj.load(std::memory_order_acquire);

    if (ptr != nullptr) {
        delete ptr;
    }
}

bool try_add_uint64(int64_t delta, std::atomic<uint64_t>& base)
{
    uint64_t expect = base.load(std::memory_order_relaxed);
    while(true)
    {
        uint64_t desire;
        if (__builtin_add_overflow(expect, delta, &desire))
        {
            return false;
        }
        if (base.compare_exchange_weak(expect, desire, std::memory_order_relaxed))
        {
            return true;
        }
        SPINLOCK_PAUSE();
    }
}

std::optional<RevertableObject::DeltaRewind>
RevertableObject::try_add_delta(const StorageDelta& delta)
{
    switch (delta.type()) {
        case DeltaType::DELETE_LAST: {
            // no op
            return DeltaRewind(RevertableBaseObject::Rewind(), delta, this);
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            StorageDeltaClass obj;
            obj.type(ObjectType::NONNEGATIVE_INT64);
            obj.nonnegative_int64()
                = delta.set_add_nonnegative_int64().set_value;

            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            int64_t d = delta.set_add_nonnegative_int64().delta;
            if (d < 0) {
                while (true) {
                    int64_t cur_value
                        = total_subtracted.load(std::memory_order_relaxed);
                    if (__builtin_add_overflow_p(
                            cur_value, d, static_cast<int64_t>(0))) {
                        return std::nullopt;
                    }

                    int64_t new_value = cur_value + d;
                    int64_t base = delta.set_add_nonnegative_int64().set_value;

                    if (base < 0 || base + new_value < 0) {
                        return std::nullopt;
                    }

                    if (total_subtracted.compare_exchange_weak(
                            cur_value, new_value, std::memory_order_relaxed)) {
                        return DeltaRewind(std::move(*res), delta, this);
                    }
                }
            } else {
                total_added.add(d);
                return DeltaRewind(std::move(*res), delta, this);
            }
            throw std::runtime_error("assert unreachable");
        }
        case DeltaType::RAW_MEMORY_WRITE: {
            StorageDeltaClass obj;
            obj.type(ObjectType::RAW_MEMORY);
            obj.data() = delta.data();

            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            return DeltaRewind(std::move(*res), delta, this);
        }
        case DeltaType::HASH_SET_INCREASE_LIMIT: {
            StorageDeltaClass obj;
            obj.type(ObjectType::HASH_SET);

            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            size_increase.fetch_add(delta.limit_increase(),
                                    std::memory_order_relaxed);

            return DeltaRewind(std::move(*res), delta, this);
        }
        case DeltaType::HASH_SET_INSERT: {
            StorageDeltaClass obj;
            obj.type(ObjectType::HASH_SET);
            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            size_t cur_size = 0;
            size_t max_size = START_HASH_SET_SIZE;

            if (committed_base) {
                cur_size = committed_base->body.hash_set().hashes.size();
                max_size = committed_base->body.hash_set().max_size;

                for (auto const& h : committed_base->body.hash_set().hashes) {
                    // TODO these will be sorted (unless we pick a homomorphic
                    // hash fn)? bin search might be faster
                    if (h == delta.hash()) {
                        return std::nullopt;
                    }
                }
            }

            cur_size
                += num_new_elts.fetch_add(1, std::memory_order_relaxed) + 1;

            if (cur_size > max_size) {
                num_new_elts.fetch_sub(1, std::memory_order_relaxed);
                return std::nullopt;
            }

            if (!new_hashes.try_insert(delta.hash())) {
                num_new_elts.fetch_sub(1, std::memory_order_relaxed);
                return std::nullopt;
            }

            return DeltaRewind(std::move(*res), delta, this);
        }
        case DeltaType::HASH_SET_CLEAR:
        {
            StorageDeltaClass obj;
            obj.type(ObjectType::HASH_SET);
            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            return DeltaRewind(std::move(*res), delta, this);
        }
        case DeltaType::ASSET_OBJECT_ADD:
        {
            StorageDeltaClass obj;
            obj.type(ObjectType::KNOWN_SUPPLY_ASSET);
            auto res = base_obj.try_set(obj);

            if (!res) {
                return std::nullopt;
            }

            int64_t d = delta.asset_delta();

            if (d < 0)
            {
                if (!try_add_uint64(d, available_asset))
                {
                    return std::nullopt;
                }
            } else
            {
                if (!try_add_uint64(d, available_asset_upperbound))
                {
                    return std::nullopt;
                }
            }
            return DeltaRewind(std::move(*res), delta, this);
        }
        default:
            throw std::runtime_error(
                "unimplemented deltatype in RevertableObject::try_set");
    }
    throw std::runtime_error("assert unreachable");
}

void
RevertableObject::commit_delta(const StorageDelta& delta)
{
    switch(delta.type())
    {
        case DeltaType::DELETE_LAST:
        {
            delete_last_committed.store(true, std::memory_order_relaxed);
            return;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD:
        {
            // no op
            return;
        }
        case DeltaType::RAW_MEMORY_WRITE:
        {
            // no op
            return;
        }
        case DeltaType::HASH_SET_INCREASE_LIMIT: {
            // no op
            return;
        }
        case DeltaType::HASH_SET_INSERT:
        {
            // no op
            return;
        }
        case DeltaType::HASH_SET_CLEAR:
        {
            hashset_clear_committed.store(true, std::memory_order_relaxed);
            while(true)
            {
                uint64_t cur_threshold = max_committed_clear_threshold.load(std::memory_order_relaxed);
                if (cur_threshold >= delta.threshold())
                {
                    return;
                }
                if (max_committed_clear_threshold.compare_exchange_strong(
                    cur_threshold, delta.threshold(), std::memory_order_relaxed))
                {
                    return;
                }
            }
        }
        case DeltaType::ASSET_OBJECT_ADD:
        {
            int64_t const& d = delta.asset_delta();

            if (d < 0)
            {
                available_asset_upperbound.fetch_add(d, std::memory_order_relaxed);
            } else
            {
                available_asset.fetch_add(d, std::memory_order_relaxed);
            }
            return;
        }
    }
    throw std::runtime_error("unknown deltatype in commit_delta()");
}

void
RevertableObject::revert_delta(const StorageDelta& delta)
{
    switch (delta.type()) {
        case DeltaType::DELETE_LAST: {
            // no op
            return;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
            int64_t d = delta.set_add_nonnegative_int64().delta;
            if (d < 0) {
                total_subtracted.fetch_sub(d, std::memory_order_relaxed);
                return;
            }

            // ok to implicitly cast pos int64 to uint64
            total_added.sub(d);
            return;
        }
        case DeltaType::RAW_MEMORY_WRITE: {
            // no op
            return;
        }
        case DeltaType::HASH_SET_INCREASE_LIMIT: {
            size_increase.fetch_sub(delta.limit_increase(),
                                    std::memory_order_relaxed);
            return;
        }
        case DeltaType::HASH_SET_INSERT: {
            num_new_elts.fetch_sub(1, std::memory_order_relaxed);
            new_hashes.erase(delta.hash());
            return;
        }
        case DeltaType::HASH_SET_CLEAR:
        {
            // no op
            return;
        }
        case DeltaType::ASSET_OBJECT_ADD:
        {
            int64_t const& d = delta.asset_delta();

            if (d < 0)
            {
                available_asset.fetch_sub(d, std::memory_order_relaxed);
            } else
            {
                available_asset_upperbound.fetch_sub(d, std::memory_order_relaxed);
            }
            return;
        }
    }
    throw std::runtime_error("assert unreachable");
}

RevertableObject::DeltaRewind::DeltaRewind(
    RevertableBaseObject::Rewind&& rewind_base,
    const StorageDelta& delta,
    RevertableObject* obj)
    : rewind_base(std::move(rewind_base))
    , delta(delta)
    , obj(obj)
{}

RevertableObject::DeltaRewind::DeltaRewind(DeltaRewind&& other)
    : rewind_base(std::move(other.rewind_base))
    , delta(other.delta)
    , obj(other.obj)
    , do_rewind(other.do_rewind)
{
    other.do_rewind = false;
}

RevertableObject::DeltaRewind&
RevertableObject::DeltaRewind::operator=(DeltaRewind&& other)
{
    if (do_rewind) {
        obj->revert_delta(delta);
    }

    rewind_base = std::move(other.rewind_base);
    delta = other.delta;
    obj = other.obj;
    do_rewind = other.do_rewind;
    other.do_rewind = false;
    return *this;
}

void
RevertableObject::DeltaRewind::commit()
{
    obj->commit_delta(delta);
    rewind_base.commit();
    do_rewind = false;
}

RevertableObject::DeltaRewind::~DeltaRewind()
{
    if (do_rewind) {
        obj->revert_delta(delta);
    }
    do_rewind = false;
}

std::optional<StorageDeltaClass>
RevertableBaseObject::commit_round_and_reset()
{
    StorageDeltaClass* base_obj_ptr = obj.load(std::memory_order_relaxed);
    tag = 0;

    if (base_obj_ptr == nullptr) {
        // obj is nullptr
        return std::nullopt;
    }
    obj = nullptr;

    required_type = base_obj_ptr->type();

    StorageDeltaClass out = *base_obj_ptr;
    delete base_obj_ptr;

    return out;
}

void
RevertableBaseObject::rewind_round()
{
    StorageDeltaClass* base_obj_ptr = obj.load(std::memory_order_relaxed);
    tag = 0;
    obj = nullptr;

    // no mod to required_type

    if (base_obj_ptr != nullptr)
    {
        delete base_obj_ptr;
    }
}

void
RevertableObject::clear_mods()
{
    total_subtracted.store(0, std::memory_order_relaxed);
    total_added.clear();

    size_increase.store(0, std::memory_order_relaxed);
    new_hashes.clear();
    num_new_elts.store(0, std::memory_order_relaxed);

    hashset_clear_committed.store(false, std::memory_order_relaxed);
    max_committed_clear_threshold.store(0, std::memory_order_relaxed);

    delete_last_committed.store(false, std::memory_order_relaxed);

    if (committed_base.has_value() && (committed_base->body.type() == ObjectType::KNOWN_SUPPLY_ASSET))
    {
        available_asset.store(committed_base->body.asset().amount, std::memory_order_relaxed);
        available_asset_upperbound.store(committed_base->body.asset().amount, std::memory_order_relaxed);
    } 
    else
    {
        available_asset.store(0, std::memory_order_relaxed);
        available_asset_upperbound.store(0, std::memory_order_relaxed);
    }
}

void
RevertableObject::commit_round()
{

    auto new_base = base_obj.commit_round_and_reset();

    if (new_base) {
        committed_base = object_from_delta_class(*new_base, committed_base);
    }

    if (!committed_base) {
        clear_mods(); // clears any inflight_deletes that may have deleted an
                      // already nexist key
                      // (only way we could get here is if we deleted an nexist key)
        return;
    }

    if (delete_last_committed.load(std::memory_order_relaxed)) {
        clear_mods();
        committed_base = std::nullopt;
        base_obj.clear_required_type();
        return;
    }

    switch (committed_base->body.type()) {
        case ObjectType::NONNEGATIVE_INT64: {
            int64_t sub = total_subtracted.load(std::memory_order_relaxed);
            committed_base->body.nonnegative_int64() += sub;

            uint64_t add = total_added.fetch_cap();

            if (__builtin_add_overflow_p(
                    committed_base->body.nonnegative_int64(),
                    add,
                    static_cast<int64_t>(0))) {
                committed_base->body.nonnegative_int64() = INT64_MAX;
            } else {
                committed_base->body.nonnegative_int64() += add;
            }
            break;
        }
        case ObjectType::RAW_MEMORY:
            // no op
            break;
        case ObjectType::HASH_SET: {
	    uint64_t size_inc = size_increase.load(std::memory_order_relaxed);
            uint64_t new_size = size_inc
                                + committed_base->body.hash_set().max_size;

            committed_base->body.hash_set().max_size
                = std::min((uint64_t)MAX_HASH_SET_SIZE, new_size);

            auto new_hashes_list = new_hashes.get_hashes();

	    if (size_inc >0)
	    {
		    new_hashes.resize(new_size);
	    }

            auto& h_list = committed_base->body.hash_set().hashes;

            h_list.insert(h_list.end(),
                          std::make_move_iterator(new_hashes_list.begin()),
                          std::make_move_iterator(new_hashes_list.end()));

            normalize_hashset(committed_base -> body.hash_set());

            if (hashset_clear_committed.load(std::memory_order_relaxed))
            {
                uint64_t threshold = max_committed_clear_threshold.load(std::memory_order_relaxed);

                clear_hashset(committed_base -> body.hash_set(), threshold);
            }

            break;
        }
        case ObjectType::KNOWN_SUPPLY_ASSET:
        {
            committed_base -> body.asset().amount = available_asset.load(std::memory_order_relaxed);

            utils::print_assert(
                committed_base -> body.asset().amount == available_asset_upperbound.load(std::memory_order_relaxed),
                "asset value vs. upperbound mismatch in commit");
        }
        break;
        default:
            throw std::runtime_error("unimplemented object type in "
                                     "RevertableObject::commit_round()");
    }
    clear_mods();
}

void 
RevertableObject::rewind_round()
{
    base_obj.rewind_round();

    clear_mods();
}

RevertableObject::RevertableObject()
    : base_obj()
    , total_subtracted(0)
    , total_added()
    , size_increase(0)
    , new_hashes(START_HASH_SET_SIZE)
    , num_new_elts(0)
    , hashset_clear_committed(false)
    , max_committed_clear_threshold(0)
    , delete_last_committed(false)
    , available_asset(0)
    , available_asset_upperbound(0)
    , committed_base(std::nullopt)
{}

RevertableObject::RevertableObject(const StorageObject& committed_base_)
    : base_obj(committed_base_)
    , total_subtracted(0)
    , total_added()
    , size_increase(0)
    , new_hashes(0)
    , num_new_elts(0)
    , hashset_clear_committed(false)
    , max_committed_clear_threshold(0)
    , delete_last_committed(false)
    , available_asset(0)
    , available_asset_upperbound(0)
    , committed_base(committed_base_)
{
    if (committed_base -> body.type() == ObjectType::HASH_SET) {
        new_hashes.resize(committed_base->body.hash_set().max_size);
    }
    else if (committed_base -> body.type() == ObjectType::KNOWN_SUPPLY_ASSET) {
        available_asset.store(committed_base -> body.asset().amount, std::memory_order_relaxed);
        available_asset_upperbound.store(committed_base -> body.asset().amount, std::memory_order_relaxed);
    }
}

} // namespace scs
