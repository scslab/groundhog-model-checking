#pragma once

#include "libimc.h"

#ifndef OX64
#define N_WORKERS 16
#else
#define N_WORKERS 3
#endif
#define MAX_DEPTH 128

struct partial_path {
    // The choices made along this path
    choice_t choices[MAX_DEPTH];
    // How long is this path?
    int n_choices;
    // Once choices[n_choices] == last_choice_max, this partial path is done
    int last_choice_max;
    // Stack
    struct partial_path *stack_next;
};

struct __attribute__((__packed__)) message {
    uint8_t max_n;
    hash_t hash;
};

struct __attribute__((__packed__)) message_bundle {
    struct  __attribute__((__packed__)) {
        size_t n_messages;
        uint8_t exit_status;
    } header;
    struct message messages[MAX_DEPTH];
};

// [MASTER] Any target-specific initialization that must be done
void initialize_workers();

// [MASTER] Check for a message from worker @worker_idx
int read_worker_bundle(struct message_bundle *bundle, int worker_idx);

// [WORKER] Send a batch of messages to the master
// Can only be called once; the worker will exit afterwards.
void send_worker_bundle(struct message_bundle *bundle);

// [MASTER] Spawn worker @worker_idx to run @worker_spawn(data)
// Memory semantics can be either:
//
//      1. As if the program was rerun from scratch, starting at @runner
//      2. Fork semantics from the same state as the master
extern void worker_spawn(void *data);
int spawn_worker(void *data, size_t n_data, int worker_idx);

extern void launch_master();
