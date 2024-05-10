#include "minimal_stdlib.h"
#include "_libimc.h"

struct partial_path CURRENT_PATH;
int CURRENT_N_CHOICES;
struct message_bundle BUNDLE;

choice_t choose(choice_t n, hash_t hash) {
    if (CURRENT_N_CHOICES++ < CURRENT_PATH.n_choices)
        return CURRENT_PATH.choices[CURRENT_N_CHOICES - 1];

    if (CURRENT_N_CHOICES >= MAX_DEPTH) check_exit_normal();

    BUNDLE.messages[BUNDLE.header.n_messages] = (struct message){
        .max_n = n,
        .hash = hash
    };
    BUNDLE.header.n_messages++;

    return 0;
}

void report_error() {
    printf("ERROR found!\n");
    BUNDLE.header.exit_status = 1;
    send_worker_bundle(&BUNDLE);
    exit(1);
}

void check_exit_normal() {
    // printf("Reporting normal! %d; %d\n", MESSAGE.n_new, CURRENT_PATH.n_choices);
    BUNDLE.header.exit_status = 0;
    send_worker_bundle(&BUNDLE);
    exit(0);
}

void worker_spawn(void *data) {
    memset(&BUNDLE, 0, sizeof(BUNDLE));
    struct partial_path *partial_path = data;
    CURRENT_PATH = *partial_path;

#if 0
    for (int i = 0; i < CURRENT_PATH.n_choices; i++) {
        print_num(CURRENT_PATH.choices[i]);
        printf(" ");
    }
    printf("\n");
#endif

    check_main();
    check_exit_normal();
}
