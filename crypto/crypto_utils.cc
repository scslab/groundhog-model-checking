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

#include "crypto/crypto_utils.h"

#include "pedersen_ffi/pedersen.h"

#include <atomic>
#include <cstdio>
#include <sodium.h>
#include <stdexcept>

namespace scs {

std::atomic<bool> hash_key_init = false;
uint8_t* HASH_KEY = nullptr;

void
initialize_crypto()
{
    int res = sodium_init();
    if (res == -1) {
        throw std::runtime_error("failed to init sodium");
    }
    bool expect = false;
    if (hash_key_init.compare_exchange_strong(expect, true)) {
        HASH_KEY = new uint8_t[crypto_shorthash_KEYBYTES];
        crypto_shorthash_keygen(HASH_KEY);
    }

    init_pedersen();

    std::printf("initialized sodium\n");
}

uint32_t
shorthash(const uint8_t* data, uint32_t data_len, const uint32_t modulus)
{
    static_assert(crypto_shorthash_BYTES == 8);

    uint64_t hash_out;

    if (crypto_shorthash(
            reinterpret_cast<uint8_t*>(&hash_out), data, data_len, HASH_KEY)
        != 0) {
        throw std::runtime_error("shorthash fail");
    }

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
    return ((hash_out & 0xFFFF'FFFF) /* 32 bits */
            * static_cast<uint64_t>(modulus))
           >> 32;
}

bool
check_sig_ed25519(PublicKey const& pk, Signature const& sig, std::vector<uint8_t> const& msg)
{
    return crypto_sign_verify_detached(
        sig.data(), msg.data(), msg.size(), pk.data()) == 0;

}

Signature
sign_ed25519(SecretKey const& sk, uint8_t const* msg, uint32_t msg_len)
{
    Signature out;
    if (crypto_sign_detached(
        out.data(), //signature
        nullptr, //optional siglen ptr
        msg, //msg
        msg_len, //msg len
        sk.data())) //sk
    {
        throw std::runtime_error("failed to sign msg ed25519");
    }

    return out;
}


std::pair<SecretKey, PublicKey> 
deterministic_key_gen(uint64_t seed)
{
    std::array<uint64_t, 4> seed_bytes; // 32 bytes
    seed_bytes.fill(0);
    seed_bytes[0] = seed;

    SecretKey sk;
    PublicKey pk;

    if (crypto_sign_seed_keypair(pk.data(), sk.data(), reinterpret_cast<unsigned char*>(seed_bytes.data()))) {
        throw std::runtime_error("sig gen failed!");
    }

    return std::make_pair(sk, pk);
}

} // namespace scs
