
#include "libptimc.h"

#include <cstdint>

#include "hash_set/atomic_set.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"
#include "mtt/ephemeral_trie/atomic_ephemeral_trie.h"

using trie_t = trie::AtomicTrie<trie::EmptyValue, trie::UInt64Prefix, trie::EphemeralTrieMetadataBase, 5>;

static std::unique_ptr<trie_t> t;

static void resetter() {
    t.reset();
}

void *insert_worker1(void *ptr) {
    trie::AtomicTrieReference ref(*t);
    assert(ref.insert(trie::UInt64Prefix{0}));
    assert(!ref.insert(trie::UInt64Prefix{0}));
    return NULL;
}

void *insert_worker2(void *ptr) {
    trie::AtomicTrieReference ref(*t);
    assert(ref.insert(trie::UInt64Prefix{1}));
    return NULL;
}


void imc_check_main(void) {
    scs::yield_config.EPHEMERALTRIE_YIELD = true;

    register_resetter(resetter);
    t = std::make_unique<trie_t>();

    imcthread_t t1, t2;
    imcthread_create(&t1, NULL, insert_worker1, 0);
    imcthread_create(&t2, NULL, insert_worker2, 0);

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);

    assert(t->deep_sizecheck() == 2);
}
