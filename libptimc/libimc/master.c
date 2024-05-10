#include "minimal_stdlib.h"
#include "_libimc.h"

// From worker.c
extern void worker_spawn(void *data);
extern struct partial_path CURRENT_PATH;

///////// HASH SET
struct hash_set {
    hash_t *table;
    size_t cap;
    size_t count;
};
struct hash_set *init_hash_set() {
    struct hash_set *hash_set = malloc(sizeof(struct hash_set));
    hash_set->cap = 1024;
    hash_set->table = calloc(hash_set->cap, sizeof(hash_set->table[0]));
    hash_set->count = 0;
    return hash_set;
}
int check_hash_set(struct hash_set *set, hash_t el) {
    size_t i = el % set->cap;
    while (set->table[i]) {
        if (set->table[i++] == el) return 1;
        if (i == set->cap) i = 0;
    }
    return 0;
}
void insert_hash_set_(struct hash_set *set, hash_t el) {
    size_t i = el % set->cap;
    while (set->table[i++]) if (i == set->cap) i = 0;
    set->table[i-1] = el;
}

void insert_hash_set(struct hash_set *set, hash_t el) {
    if (((set->count + 1) * 2) >= set->cap) {
        struct hash_set old_set = *set;
        set->cap *= 2;
        set->table = calloc(set->cap, sizeof(set->table[0]));
        for (size_t i = 0; i < old_set.cap; i++)
            if (old_set.table[i])
                insert_hash_set_(set, old_set.table[i]);
        free(old_set.table);
    }
    insert_hash_set_(set, el);
    set->count++;
}

/* Basic setup:
 * - Master task forks a worker to run the program.
 * - When the worker has to make a choice, it reports the possible choices and
 *   which one it made back to the master.
 * - Master keeps track of a list of partial choices. Every time a worker dies,
 *   master picks one of those partial choices and spins up a new worker for
 *   it.
 *
 * How to incorporate state hashing?
 * - When the worker has to make a choice, it also sends back a hash of its
 *   current state. If the hash matches something already in the hash set, the
 *   master kills the worker and ignores any pending communications from it.
 */
void launch_master() {
    struct hash_set *hash_set = init_hash_set();

    int WORKER_LIVE[N_WORKERS] = {0};
    struct partial_path WORKER_PATH[N_WORKERS] = {0};
    struct partial_path *work_stack = 0;

    initialize_workers();

    // Fork off the first worker
    while (1) {
        for (int i = 0; i < N_WORKERS; i++) {
            if (!spawn_worker(&WORKER_PATH[i], sizeof(WORKER_PATH[i]), i))
                continue;
            WORKER_LIVE[i] = 1;
            goto begin;
        }
    }

begin:
    int n_stack = 0, n_kills = 0;
    for (int i = 0; ; i++) {
        // Visit all the workers. For each, check:
        // - If it has a buffered choice, update the choice stack.
        // - If it is no longer alive, pop from the choice stack and revive it
        int keep_going = 0;
        for (int i = 0; i < N_WORKERS; i++) {
            if (WORKER_LIVE[i]) {
                keep_going = 1;

                struct message_bundle bundle;
                if (!read_worker_bundle(&bundle, i)) continue;

                WORKER_LIVE[i] = 0;
                n_kills++;
#ifdef OX64
                if ((n_kills % 50) == 0) {
                    printf("N kills: "); print_num(n_kills); printf("\n");
                }
#endif

                for (int j = 0; j < bundle.header.n_messages; j++) {
                    struct message message = bundle.messages[j];

                    struct partial_path *partial_path
                        = malloc(sizeof(struct partial_path));
                    *partial_path = WORKER_PATH[i];
                    partial_path->choices[partial_path->n_choices++] = 0;
                    partial_path->last_choice_max = message.max_n;

                    if (check_hash_set(hash_set, message.hash)) break;
                    insert_hash_set(hash_set, message.hash);

                    // add it to the stack
                    partial_path->stack_next = work_stack;
                    work_stack = partial_path;
                    n_stack++;
                    WORKER_PATH[i] = *partial_path;
                }

                if (!bundle.header.exit_status) continue;

                printf("Found an error with path: ");
                for (int j = 0; j < WORKER_PATH[i].n_choices; j++) {
#ifdef OX64
                    print_num(WORKER_PATH[i].choices[j]);
#else
                    printf("%d", WORKER_PATH[i].choices[j]);
#endif
                    printf(" ");
                }
                printf("\n");
                exit(1);
            } else if (work_stack) {
                keep_going = 1;
                struct partial_path *new_path = work_stack;
                new_path->choices[new_path->n_choices - 1]++;
                if (new_path->choices[new_path->n_choices - 1]
                       == new_path->last_choice_max) {
                    work_stack = work_stack->stack_next;
                    n_stack--;
                    free(new_path);
                    continue;
                }
                if (!spawn_worker(new_path, sizeof(*new_path), i)) {
                    new_path->choices[new_path->n_choices - 1]--;
                    continue;
                }
                WORKER_LIVE[i] = 1;
                WORKER_PATH[i] = *new_path;
            }
        }

        if (!keep_going) {
#ifdef OX64
            printf("Done; in total killed: "); print_num(n_kills); printf("\n");
#else
            printf("Done; in total killed: "); printf("%d", n_kills); printf("\n");
#endif
            while (1) ;
        }
    }
}

void launch_replay(choice_t *path) {
    CURRENT_PATH.n_choices = MAX_DEPTH;
    memcpy(CURRENT_PATH.choices, path, MAX_DEPTH * sizeof(choice_t));
    check_main();
    printf("Finished execution, but probably shouldn't have ...");
    exit(0);
}
