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

#include <cstdint>
#include <vector>
#include <utility>

#include "xdr/types.h"

namespace scs {

void __attribute__((constructor)) initialize_crypto();

// uses shared key randomly generated at startup
uint32_t
shorthash(const uint8_t* data, uint32_t data_len, const uint32_t modulus);

bool
check_sig_ed25519(PublicKey const& pk, Signature const& sig, std::vector<uint8_t> const& msg);

Signature
sign_ed25519(SecretKey const& sk, uint8_t const* msg, uint32_t msg_len);

template<typename T>
Signature
sign_ed25519(SecretKey const& sk, T const& msg)
{
	return sign_ed25519(sk, msg.data(), msg.size());
}

std::pair<SecretKey, PublicKey> 
deterministic_key_gen(uint64_t seed);

} // namespace scs
