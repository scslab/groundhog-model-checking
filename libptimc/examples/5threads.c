#include "libptimc.h"
#include <assert.h>

void *worker(void *_) {
    // printf("Running worker %llu\n", (size_t)_);
    return 0;
}

void imc_check_main(void) {
    imcthread_t ts[10];
    int n = (sizeof ts) / (sizeof ts[0]);
    for (size_t i = 0; i < n; i++)
        imcthread_create(&ts[i], NULL, worker, (void *)i);

    for (size_t i = 0; i < n; i++) {
        // printf("Starting the join on %llu\n", i);
        imcthread_join(ts[i], NULL);
        // printf("Finished the join on %llu\n", i);
    }
    // printf("Done!\n");
    // printf("---------------------\n");
}
