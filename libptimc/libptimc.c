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
    makecontext(&thread->ctx, imcthread_entry_point, 1, thread->id);
}

int imcthread_yield(void) {
    // pauses the current thread and returns to the master thread
    assert(CURRENT_THREAD);
    swapcontext(&(CURRENT_THREAD->ctx), &MASTER_CTX);
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

    thread->fn(thread->arg);

    thread->state = THREAD_STATE_DEAD;

    imcthread_yield();
}
