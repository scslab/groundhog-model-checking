#include "libptimc.h"

static struct imcthread **THREADS = 0;
static size_t N_THREADS = 0;
static struct imcthread *CURRENT_THREAD = 0;
static ucontext_t MASTER_CTX;

static void switch_to_thread(struct imcthread *thread);
static void imcthread_entry_point(int tid);

int imcthread_create(imcthread_t *threadp,
                     imcthread_attr_t *attr,
                     void *(*start_routine)(void *),
                     void *arg) {
    assert(!attr);
    THREADS = realloc(THREADS, (++N_THREADS) * sizeof(THREADS[0]));
    THREADS[N_THREADS - 1] = calloc(1, sizeof(struct imcthread));

    struct imcthread *thread = THREADS[N_THREADS - 1];
    *threadp = thread;
    thread->fn = start_routine;
    thread->arg = arg;
    thread->state = THREAD_STATE_FETUS;
    thread->id = N_THREADS - 1;

    assert(!getcontext(&thread->ctx));
    thread->ctx.uc_stack.ss_size = 4 * 1024 * 1024;
    thread->ctx.uc_stack.ss_sp = malloc(thread->ctx.uc_stack.ss_size);
    thread->ctx.uc_link = 0;
    makecontext(&thread->ctx, imcthread_entry_point, 1, thread->id);

    return 0;
}

int imcthread_yieldh(hash_t hash) {
    // pauses the current thread and returns to the master thread
    assert(CURRENT_THREAD);
    struct imcthread *thread = CURRENT_THREAD;

    if (thread->state == THREAD_STATE_DEAD) {
        for (size_t i = 0; i < N_THREADS; i++)
            if (THREADS[i]->waiting_on == thread)
                THREADS[i]->waiting_on = 0;
    }

    CURRENT_THREAD = 0;
    swapcontext(&(thread->ctx), &MASTER_CTX);

    return 0;
}
int imcthread_yield(void) { imcthread_yieldh(0); }

static void *_imc_check_main(void *_) { imc_check_main(); return 0; }
void check_main() {
    imcthread_t _thread;
    imcthread_create(&_thread, NULL, _imc_check_main, NULL);

    while (1) {
        int n_alive = 0, n_avail = 0;
        for (int i = 0; i < N_THREADS; i++) {
            if (THREADS[i]->state == THREAD_STATE_DEAD) continue;
            n_alive++;
            if (THREADS[i]->waiting_on) continue;
            n_avail++;
        }
        // check that we're not in a deadlock
        if (!n_avail) { assert(!n_alive); break; }

        int count = choose(n_avail, 0);
        for (int i = 0; i < N_THREADS; i++) {
            if (THREADS[i]->state == THREAD_STATE_DEAD) continue;
            if (THREADS[i]->waiting_on) continue;
            if (count--) continue;
            switch_to_thread(THREADS[i]);
            break;
        }
    }
}

int imcthread_joinh(imcthread_t thread, void **retval, hash_t hash) {
    if (thread->state != THREAD_STATE_DEAD) {
        CURRENT_THREAD->waiting_on = thread;
        imcthread_yield();
    }

    if (retval) *retval = thread->retval;
    return 0;
}
int imcthread_join(imcthread_t thread, void **retval) {
    return imcthread_joinh(thread, retval, 0);
}

static void switch_to_thread(struct imcthread *thread) {
    assert(!CURRENT_THREAD);
    assert(thread->state != THREAD_STATE_DEAD);

    CURRENT_THREAD = thread;
    swapcontext(&MASTER_CTX, &(thread->ctx));
}

static void imcthread_entry_point(int tid) {
    struct imcthread *thread = THREADS[tid];
    thread->state = THREAD_STATE_ALIVE;
    CURRENT_THREAD = thread;

    void *result = thread->fn(thread->arg);

    thread->retval = result;
    thread->state = THREAD_STATE_DEAD;

    imcthread_yield();
}
