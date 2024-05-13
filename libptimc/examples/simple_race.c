#include "libptimc.h"
#include <assert.h>

static int COUNTER = 0;

hash_t state_hasher() { return (hash_t)COUNTER; }

void *worker(void *_) {
    int local_counter = COUNTER;
    local_counter++;
    imcthread_yield();
    COUNTER = local_counter;
}

void imc_check_main(void) {
    register_hasher(state_hasher);

    imcthread_t t1;
    imcthread_t t2;
    // printf("About to create two threads!\n");
    imcthread_create(&t1, NULL, worker, 0);
    imcthread_create(&t2, NULL, worker, 0);
    // printf("Threads created!\n");

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);

    assert(COUNTER == 2);
}
