#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>

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

    ucontext_t ctx;
} *imcthread_t;

typedef int imcthread_attr_t;

int imcthread_create(imcthread_t *thread,
                     // only support NULL attr for now
                     imcthread_attr_t *attr,
                     void *(*start_routine)(void *),
                     void *arg);

int imcthread_yield(void);

// start model checking
void imcthread_start(void);
