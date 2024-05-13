
#include "libptimc/libptimc.h"

#include <cstdint>

#include "utils/atomic_uint128.h"
#include "config/yield_config.h"

static scs::AtomicUint128 counter;

void *worker(void *_) {
    counter.add(UINT64_MAX);
    return NULL;
}

void *sub_worker(void *_) {
    counter.add(UINT64_MAX);
    counter.sub(UINT64_MAX);
    return NULL;
}

void imc_check_main(void) {
    scs::yield_config.UINT128_YIELD = true;

    imcthread_t t1, t2, t3;
    // printf("About to create two threads!\n");
    imcthread_create(&t1, NULL, worker, 0);
    imcthread_create(&t2, NULL, worker, 0);
    imcthread_create(&t3, NULL, sub_worker, 0);

    // printf("Threads created!\n");

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
    imcthread_join(t3, NULL);

    uint64_t lowbits, highbits;
    counter.test_read_total(lowbits, highbits);

    assert(lowbits == UINT64_MAX - 1);
    assert(highbits == 1);


}
