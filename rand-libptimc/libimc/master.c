#include "_libimc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void launch_master() {
    int workers[N_WORKERS];
    for (int i = 0; i < N_WORKERS; i++)
        if (!(workers[i] = fork()))
            launch_worker(i);

    // wait for a worker to die
    wait(0);

    for (int i = 0; i < N_WORKERS; i++) kill(i, SIGTERM);

    exit(0);
}

int main(int argc, char **argv) {
    extern int sscanf(const char *str, const char *format, ...);
    if (argc > 1) {
        printf("This version doesn't yet support replay.\n");
        exit(1);
    } else {
        launch_master();
    }
    return 0;
}
