
#include "libptimc.h"

#include <cstdint>

#include "utils/atomic_uint128.h"
#include "config/yield_config.h"

#include <memory>

static std::unique_ptr<scs::AtomicUint128> counter;

static void resetter() {
    counter.reset();
}

void *worker(void *_) {
    counter->buggy_add(10);
    return NULL;
}

void imc_check_main(void) {
    scs::yield_config.UINT128_YIELD = true;

    register_resetter(resetter);
    counter = std::make_unique<scs::AtomicUint128>();

    counter->add(UINT64_MAX - 15);

    imcthread_t t1;
    imcthread_t t2;
    // printf("About to create two threads!\n");
    imcthread_create(&t1, NULL, worker, 0);
    imcthread_create(&t2, NULL, worker, 0);
    // printf("Threads created!\n");

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);

    uint64_t lowbits, highbits;
    counter->test_read_total(lowbits, highbits);

    assert(lowbits == 5);
    assert(highbits == 1);
}
