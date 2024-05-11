#include "libptimc.h"
#include <stdint.h>

static uint64_t counter;

void *worker(void *_) {
    counter += UINT32_MAX;
    printf("Finished worker; counter value is %llu\n", counter);
    return NULL;
}

void *sub_worker(void *_) {
    counter += UINT32_MAX;
    counter -= UINT32_MAX;
    return NULL;
}

void imc_check_main(void) {
    printf("Starting main; counter value is %llu\n", counter);

    imcthread_t t1, t2, t3;
    imcthread_create(&t1, NULL, worker, 0);
    imcthread_create(&t2, NULL, worker, 0);
    imcthread_create(&t3, NULL, sub_worker, 0);

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);
    imcthread_join(t3, NULL);

    assert(counter == ((uint64_t)UINT32_MAX) + ((uint64_t)UINT32_MAX));
}
