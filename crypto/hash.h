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

#include <stdexcept>

#include <sodium.h>
#include <xdrpp/marshal.h>

namespace scs {

template<typename T>
concept XdrType = requires(const T object)
{
    xdr::xdr_to_opaque(object);
};

//! Hash an xdr (serializable) type.
//! Must call sodium_init() prior to usage.
template<XdrType xdr_type>
Hash
hash_xdr(const xdr_type& value)
{
    Hash hash_buf;
    auto serialized = xdr::xdr_to_opaque(value);

    if (crypto_generichash(hash_buf.data(),
                           hash_buf.size(),
                           serialized.data(),
                           serialized.size(),
                           NULL,
                           0)
        != 0) {
        throw std::runtime_error("error in crypto_generichash");
    }
    return hash_buf;
}

[[maybe_unused]]
static Hash
hash_vec(std::vector<uint8_t> const& bytes)
{
    Hash hash_buf;

    if (crypto_generichash(hash_buf.data(),
                           hash_buf.size(),
                           bytes.data(),
                           bytes.size(),
                           NULL,
                           0)
        != 0) {
        throw std::runtime_error("error in crypto_generichash");
    }
    return hash_buf;
}

[[maybe_unused]]
static void
hash_raw(const uint8_t* data, size_t len, uint8_t* hash_out)
{
    if (crypto_generichash(hash_out,
                           sizeof(Hash),
                           data,
                           len,
                           NULL,
                           0)
        != 0) {
        throw std::runtime_error("error in crypto_generichash");
    }
}

} // namespace scs
