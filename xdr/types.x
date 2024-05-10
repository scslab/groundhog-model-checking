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

namespace scs
{

typedef unsigned int uint32;
typedef int int32;

typedef unsigned hyper uint64;
typedef hyper int64;

typedef opaque uint128[16];
typedef opaque uint256[32];

typedef opaque Signature[64]; //ed25519 sig len is 512 bits
typedef opaque PublicKey[32]; //ed25519 key len is 256 bits
typedef opaque Hash[32]; // 256 bit hash, i.e. output of sha256
typedef opaque SecretKey[64]; //ed25519 secret key len is 64 bytes, at least on libsodium

typedef opaque Address[32]; // 256 bit contract addresses

typedef opaque InvariantKey[32]; // arbitrary 32 bytes.

typedef opaque AddressAndKey[64]; // in global storage: [contract address + invariant key] = 64 bytes
// Contracts are free to set their own conventions for keys.

typedef opaque Contract<>;

typedef opaque TransactionLog<>;

const MAX_STACK_BYTES = 65536;

// crypto parameters
const MAX_HASH_LEN = 512;

} /* scs */
