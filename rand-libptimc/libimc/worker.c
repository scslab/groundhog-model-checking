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

static size_t WORKER_I;

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
        assert(n == NODE->n_children);
        NODE = NODE->children + NODE->pos;
        return NODE->choice;
    }

    // this node is unexplored
    NODE->n_children = n;
    NODE->pos = n;
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
    static size_t count = 0;
    count++;

    static time_t last_time = 0;
    if ((time(0) - last_time) > 2) {
        last_time = time(0);
        printf("Worker %2d has visited %10d\n", WORKER_I, count);
    }

    while (1) {
        struct tree_node *parent = NODE->parent;
        if (!parent) {
            printf("Exhaustive search completed!\n");
            exit(0);
        }

        parent->pos++;
        if (parent->pos < parent->n_children) break;

        free(parent->children);
        NODE = parent;
    }
    NODE = ROOT;
}

static jmp_buf RESET_JMP;

void report_error() {
    printf("ERROR found!\n");
    // might be in a signal handler
    _exit(1);
}

void check_exit_normal() {
    siglongjmp(RESET_JMP, 1);
}

static void sighandler(int which) {
    printf("Got signal: %d\n", which);
    report_error();
}

void launch_worker(unsigned int i) {
    WORKER_I = i;
    srand(i + 24);

    // (0) set up signal handlers
    struct sigaction action;
    action.sa_handler = sighandler;
    sigemptyset(&(action.sa_mask));
    action.sa_flags = 0;
    assert(!sigaction(SIGABRT, &action, 0));
    assert(!sigaction(SIGSEGV, &action, 0));

    // (1) set a setjmp for resetting the search
    if (sigsetjmp(RESET_JMP, 1)) search_increment();
    for (struct resetter *r = RESETTERS; r; r = r->next)
        r->fn();

    // (2) start the program
    check_main();

    // (3) trigger a reset
    check_exit_normal();
}
