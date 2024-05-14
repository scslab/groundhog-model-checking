#include "_libimc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>

static int IS_REPLAY;
static size_t WORKER_I;
static size_t N_BRANCHES, N_BUGS;
static void send_progress(void) {
    tell_master((struct message){
        .message_type = MSG_PROGRESS,
        .n_branches = N_BRANCHES,
        .n_bugs = N_BUGS
    }, WORKER_I);
    N_BRANCHES = N_BUGS = 0;
}

struct resetter {
    void (*fn)(void);
    struct resetter *next;
};
static struct resetter *RESETTERS;
void register_resetter(void (*fn)(void)) {
    for (struct resetter *r = RESETTERS; r; r = r->next)
        if (r->fn == fn)
            return;
    struct resetter *resetter = malloc(sizeof(struct resetter));
    resetter->fn = fn;
    resetter->next = RESETTERS;
    RESETTERS = resetter;
}

// The worker keeps track of a search tree. Each node in the tree contains:
//
// (0) The specific option that was chosen at that node
// (1) An array of the choices we have explored there so far, each one
//     associated with a specific child node.
// (2) The choice argument, i.e., how many children it has
//
// The search itself is a simple depth-first search. Once we explore all
// children of a node, we can free it.

// at each 'choose', we look at the current node's 'pos'.
struct tree_node {
    int choice;
    // @n_children == 0 means this node hasn't been explored yet.
    // note we auto-handle choose(1), so n_children > 2 if it's > 0.
    int n_children;

    // if @n_children > 0, @pos should always point to a proper child in
    // @children.
    int pos;
    struct tree_node *children;
    struct tree_node *parent;
};

struct tree_node *ROOT, *NODE;

choice_t choose(choice_t n, hash_t hash) {
    if (n == 1) return 0;

    if (!NODE) {
        // this is the first choice; give it a node
        ROOT = NODE = calloc(1, sizeof(struct tree_node));
    }
    assert(ROOT);

    if (NODE->n_children) {
        NODE = NODE->children + NODE->pos;
        return NODE->choice;
    }

    // this node is unexplored
    NODE->n_children = n;
    NODE->children = calloc(n, sizeof(NODE->children[0]));

    choice_t *options = calloc(n, sizeof(options));

    for (choice_t i = 0; i < n; i++) options[i] = i;
    for (choice_t i = 0; i < n; i++) {
        size_t swap_with = i + (rand() % (n - i));
        choice_t tmp = options[i];
        options[i] = options[swap_with];
        options[swap_with] = tmp;
    }

    for (int i = 0; i < n; i++) {
        NODE->children[i].choice = options[i];
        NODE->children[i].parent = NODE;
    }

    NODE->pos = 0;

    return choose(n, hash);
}

static void search_increment(void) {
    // finish off this branch
    while (1) {
        struct tree_node *parent = NODE->parent;
        if (!parent) {
            send_progress();
            struct message death_msg = (struct message){
                .message_type = MSG_CAN_I_DIE,
            };
            tell_master(death_msg, WORKER_I);
            // printf("Trying to die...\n");
            while (1) {
                struct message message = hear_master(WORKER_I);
                switch (message.message_type) {
                case MSG_NONE: break;
                case MSG_PLEASE_SPLIT:
                    // printf("No, I can't split!\n");
                    tell_master((struct message){MSG_NO_SPLIT}, WORKER_I);
                    tell_master(death_msg, WORKER_I);
                    break;
                case MSG_OK_DIE:
                    // printf("Yay, I can die!\n");
                    exit(0);
                    break;
                case MSG_NO_DIE:
                    // printf("Sad, I can't die :(\n");
                    break;
                default:
                   printf("'Unknown message type %d\n", message.message_type);
                   break;
                }
            }
            assert(!"Shouldn't reach here\n");
        }

        parent->pos++;
        if (parent->pos < parent->n_children) break;

        free(parent->children);
        NODE = parent;
    }
    NODE = ROOT;
}

static void try_split(struct message message) {
    struct tree_node *node = ROOT;
    while (node) {
        if (!node->n_children || node->pos == (node->n_children - 1)) {
            node = node->children + node->pos;
            continue;
        }

        send_progress();
        if (fork()) { // parent
            if (fork()) {
                // let the parent reap me
                exit(0);
            } else {
                node->n_children--;
                tell_master((struct message){MSG_DID_SPLIT}, WORKER_I);
            }
        } else { // child
            WORKER_I = message.new_id;
            node->pos = node->n_children - 1;
            assert(node->pos < node->n_children);
        }
        return;
    }
    tell_master((struct message){MSG_NO_SPLIT}, WORKER_I);
}

static jmp_buf RESET_JMP;

void report_error() {
    if (IS_REPLAY) {
        fprintf(stderr, "Done with replay!\n");
        siglongjmp(RESET_JMP, 1);
    }

    N_BUGS++;

    char tmpname[128];
    strcpy(tmpname, "bugs/XXXXXX");
    mkstemp(tmpname);
    FILE *f = fopen(tmpname, "w");

    struct tree_node *node = ROOT;
    while (node) {
        fprintf(f, "%lu\n", node->choice);
        node = node->children + node->pos;
    }

    fclose(f);

    fprintf(stderr, "Error written to %s\n", tmpname);

    check_exit_normal();
}

void check_exit_normal() {
    if (IS_REPLAY) {
        fprintf(stderr, "Replay finished with normal exit.\n");
        siglongjmp(RESET_JMP, 1);
    }

    N_BRANCHES++;
    if ((N_BRANCHES + 1) % 500 == 0)
        send_progress();
    siglongjmp(RESET_JMP, 1);
}

static void sighandler(int which) {
    printf("Got signal: %d\n", which);
    report_error();
}

void launch_worker(unsigned int i) {
    printf("Got worker!\n");

    WORKER_I = i;
    srand(i + 24);

    // (0) set up signal handlers
    struct sigaction action;
    action.sa_handler = sighandler;
    sigemptyset(&(action.sa_mask));
    action.sa_flags = 0;
    assert(!sigaction(SIGABRT, &action, 0));
    // assert(!sigaction(SIGSEGV, &action, 0));

    // (1) set a setjmp for resetting the search
    if (sigsetjmp(RESET_JMP, 1)) search_increment();
    for (struct resetter *r = RESETTERS; r; r = r->next)
        r->fn();

    // (1b) check for commands
    struct message message = hear_master(WORKER_I);
    switch (message.message_type) {
    case MSG_NONE:
        break;

    case MSG_PLEASE_SPLIT:
        try_split(message);
        break;

    default:
       printf("Unknown message type %d\n", message.message_type);
       break;
    }

    // (2) start the program
    check_main();

    // (3) trigger a reset
    check_exit_normal();
}

void worker_replay(char *path) {
    if (sigsetjmp(RESET_JMP, 1)) return;
    IS_REPLAY = 1;

    FILE *f = fopen(path, "r");
    assert(f);
    NODE = ROOT = calloc(1, sizeof(*ROOT));
    while (1) {
        size_t choice = 0;
        if (fscanf(f, " %lu", &choice) != 1) break;

        NODE->n_children = 2;
        NODE->children = calloc(2, sizeof(NODE->children[0]));
        NODE->children[0].choice = choice;
        NODE->children[0].parent = NODE;

        NODE = NODE->children;
    }
    NODE = ROOT->children;
    check_main();
    check_exit_normal();
}
