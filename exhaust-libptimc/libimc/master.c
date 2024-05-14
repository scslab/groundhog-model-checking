#include "_libimc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>

int MASTER2WORKER_RD[N_WORKERS];
int MASTER2WORKER_WR[N_WORKERS];
int WORKER2MASTER_RD[N_WORKERS];
int WORKER2MASTER_WR[N_WORKERS];

void launch_master() {
    assert(!prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0));

    // set up the control pipes
    for (int i = 0; i < N_WORKERS; i++) {
        int ends[2];
        pipe2(ends, O_NONBLOCK);
        MASTER2WORKER_RD[i] = ends[0];
        MASTER2WORKER_WR[i] = ends[1];
        pipe2(ends, O_NONBLOCK);
        WORKER2MASTER_RD[i] = ends[0];
        WORKER2MASTER_WR[i] = ends[1];
    }

    int worker_alive[N_WORKERS] = {0};
    size_t dead_count = 0;

    // launch an initial worker
    if (!fork()) {
        launch_worker(0);
        exit(0);
    }

    worker_alive[0] = 1;
    while (1) {
        // First: handle any workers that want to die
        for (int i = 0; i < N_WORKERS; i++) {
            if (!worker_alive[i]) continue;
            struct message message = hear_worker(i);
            switch (message.message_type) {
                case MSG_CAN_I_DIE:
                    dead_count += message.count;
                    worker_alive[i] = 0;
                    tell_worker((struct message){MSG_OK_DIE, 0}, i);
                    waitpid(message.pid, NULL, 0);
                    break;

                case MSG_NONE:
                    break;

                default:
                    assert(!"Bad message!");
                    break;
            }
        }

        // Second: if we have empty slots, ask a worker to split.
        int n_alive = 0, fill_slot = 0;
        for (int i = 0; i < N_WORKERS; i++) {
            n_alive += worker_alive[i];
            if (!worker_alive[i]) fill_slot = i;
        }
        if (n_alive == 0) break;
        if (n_alive == N_WORKERS) continue;

        int which = rand() % n_alive;
        for (int i = 0; i < N_WORKERS; i++) {
            if (!worker_alive[i]) continue;
            if (which--) continue;

            tell_worker((struct message){
                .message_type = MSG_PLEASE_SPLIT,
                .new_id = fill_slot
            }, i);

            while (1) {
                struct message reply = hear_worker(i);
                switch (reply.message_type) {
                    case MSG_CAN_I_DIE:
                        tell_worker((struct message){MSG_NO_DIE}, i);
                        break;

                    case MSG_DID_SPLIT:
                        worker_alive[fill_slot] = 1;
                        goto finish_split;

                    case MSG_NO_SPLIT:
                        goto finish_split;

                    case MSG_NONE:
                        break;

                    default:
                        printf("Unknown message %d!\n", reply.message_type);
                        break;
                }
            }

finish_split: break;
        }
    }

    printf("Done! Explored %lu branches\n", dead_count);
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
