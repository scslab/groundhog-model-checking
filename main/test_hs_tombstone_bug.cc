
#include "libptimc.h"

#include <cstdint>

#include "hash_set/atomic_set.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"

#include <memory>

static std::unique_ptr<scs::AtomicSet> set;

static void resetter() {
    set.reset();
}

scs::HashSetEntry make_hs_entry(uint64_t nonce) {
    scs::HashSetEntry hs;
    hs.hash = scs::hash_xdr<uint64_t>(nonce);
    return hs;
}

void *insert_worker(void *ptr) {
    *((bool*) ptr) = set->try_insert(make_hs_entry(1), 0);
    return NULL;
}

void *insert_erase(void* ptr) {
    assert(set->try_insert(make_hs_entry(2), 0));
    set->erase(make_hs_entry(2), 0);
    return NULL;
}

void imc_check_main(void) {
    scs::yield_config.HS_YIELD = true;
    scs::hashset_bug = true;

    set = std::make_unique<scs::AtomicSet>(4);
    register_resetter(resetter);

    imcthread_t t1, t2, t3;
    bool b1 = false, b2 = false;
    imcthread_create(&t1, NULL, insert_worker, &b1);
    imcthread_create(&t2, NULL, insert_worker, &b2);
    imcthread_create(&t3, NULL, insert_erase, 0);

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
    imcthread_join(t3, NULL);

    assert(b1 ^ b2);
}
