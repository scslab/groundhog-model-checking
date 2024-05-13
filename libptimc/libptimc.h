#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "libimc.h"

enum thread_state {
    THREAD_STATE_FETUS,
    THREAD_STATE_ALIVE,
    THREAD_STATE_DEAD,
};

typedef struct imcthread {
    void *(*fn)(void *);
    void *arg;
    enum thread_state state;
    int id;
    struct imcthread *waiting_on;
    void *retval;

    ucontext_t ctx;
} *imcthread_t;

typedef int imcthread_attr_t;

int imcthread_create(imcthread_t *thread,
                     // only support NULL attr for now
                     imcthread_attr_t *attr,
                     void *(*start_routine)(void *),
                     void *arg);
int imcthread_yield(void);
int imcthread_join(imcthread_t thread, void **retval);

int imcthread_yieldh(hash_t hash);
int imcthread_joinh(imcthread_t thread, void **retval, hash_t hash);

void check_main(void);

extern void imc_check_main(void);

#ifdef __cplusplus
}
#endif
