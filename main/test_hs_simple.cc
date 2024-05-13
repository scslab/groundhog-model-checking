
#include "libptimc/libptimc.h"

#include <cstdint>

#include "hash_set/atomic_set.h"
#include "crypto/hash.h"
#include "config/yield_config.h"

#include "xdr/storage.h"

static scs::AtomicSet set(4);

scs::HashSetEntry make_hs_entry(uint64_t nonce) {
    scs::HashSetEntry hs;
    hs.hash = scs::hash_xdr<uint64_t>(nonce);
    return hs;
}

void *insert_worker(void *ptr) {
    *((bool*) ptr) = set.try_insert(make_hs_entry(1));
    return NULL;
}

void *insert_other(void* ptr) {
    uint64_t val = (uint64_t) ptr;
    assert(set.try_insert(make_hs_entry(val)));
  //  assert(set.try_insert(make_hs_entry(val + 1)));
  //  assert(set.try_insert(make_hs_entry(val + 2)));
    return NULL;
}

void imc_check_main(void) {
    scs::yield_config.HS_YIELD = true;

    imcthread_t t1, t2, t3;
    bool b1 = false, b2 = false;
    // printf("About to create two threads!\n");
    imcthread_create(&t1, NULL, insert_worker, &b1);
    imcthread_create(&t2, NULL, insert_worker, &b2);

    imcthread_create(&t3, NULL, insert_other, (void*) 5);

    // printf("Threads created!\n");

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
    imcthread_join(t3, NULL);

    assert(b1 || b2);
    assert(!(b1 && b2));

}
