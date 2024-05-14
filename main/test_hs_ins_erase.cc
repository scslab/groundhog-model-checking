
#include "libptimc.h"

#include <cstdint>

#include "hash_set/atomic_set.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"

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
    assert(set->try_insert(make_hs_entry(1), 0));
    return NULL;
}

void *insert_erase(void* ptr) {
    assert(set->try_insert(make_hs_entry(2), 0));
    set->erase(make_hs_entry(2), 0);
    return NULL;
}

void imc_check_main(void) {
    scs::yield_config.HS_YIELD = true;

    register_resetter(resetter);
    set = std::make_unique<scs::AtomicSet>(4);

    imcthread_t t1, t2;
    imcthread_create(&t1, NULL, insert_worker, 0);
    imcthread_create(&t2, NULL, insert_erase, 0);

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
}
