#pragma once

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "libimc.h"

extern int MASTER2WORKER_RD[N_WORKERS];
extern int MASTER2WORKER_WR[N_WORKERS];
extern int WORKER2MASTER_RD[N_WORKERS];
extern int WORKER2MASTER_WR[N_WORKERS];

extern void launch_worker(unsigned int i);

enum message_type {
    MSG_NONE,

    // Worker -> Master
    MSG_CAN_I_DIE,
    MSG_DID_SPLIT,
    MSG_NO_SPLIT,
    MSG_PROGRESS,

    // Master -> Worker
    MSG_PLEASE_SPLIT,
    MSG_OK_DIE,
    MSG_NO_DIE,
};

struct message {
    enum message_type message_type;
    int new_id;
    int pid;

    size_t n_branches;
    size_t n_bugs;
};

static void tell_pipe(int fd, uint8_t *ptr, size_t n) {
    while (n) {
        ssize_t n_sent = write(fd, ptr, n);
        if (n_sent == EAGAIN || n_sent == EWOULDBLOCK) n_sent = 0;
        ptr += n_sent;
        n -= n_sent;
    }
}

static void hear_pipe(int fd, uint8_t *ptr, size_t n) {
    int any = 0;
    while (n) {
        ssize_t n_read = read(fd, ptr, n);
        if (n_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            n_read = 0;
        ptr += n_read;
        n -= n_read;
        if (!any && !n_read) return;
        any = 1;
    }
}

static void tell_worker(struct message message, int idx) {
    tell_pipe(MASTER2WORKER_WR[idx], (uint8_t*)&message, sizeof(message));
}

static void tell_master(struct message message, int idx) {
    tell_pipe(WORKER2MASTER_WR[idx], (uint8_t*)&message, sizeof(message));
}

static struct message hear_worker(int idx) {
    struct message message = {0};
    hear_pipe(WORKER2MASTER_RD[idx], (uint8_t*)&message, sizeof(message));
    return message;
}

static struct message hear_master(int idx) {
    struct message message = {0};
    hear_pipe(MASTER2WORKER_RD[idx], (uint8_t*)&message, sizeof(message));
    return message;
}
