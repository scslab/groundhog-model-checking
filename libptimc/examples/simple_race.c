#include "libptimc.h"

static int COUNTER = 0;

void *worker(void *_) {
    int local_counter = COUNTER;
    local_counter++;
    imcthread_yield();
    COUNTER = local_counter;
}

void imc_check_main(void) {
    imcthread_t t1;
    imcthread_t t2;
    // printf("About to create two threads!\n");
    imcthread_create(&t1, NULL, worker, 0);
    imcthread_create(&t2, NULL, worker, 0);
    // printf("Threads created!\n");

    imcthread_join(t1, NULL);
    imcthread_join(t2, NULL);

    imcassert(COUNTER == 2);
}
