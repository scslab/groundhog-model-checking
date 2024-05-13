#include "_libimc.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

extern void launch_replay(choice_t *path);
int main(int argc, char **argv) {
    extern int sscanf(const char *str, const char *format, ...);
    if (argc > 1) {
        choice_t *path = calloc(argc - 1, sizeof(choice_t));
        for (int i = 1; i < argc; i++)
            sscanf(argv[i], "%c", &path[i - 1]);
        launch_replay(path);
    } else {
        launch_master();
    }
    return 0;
}

int WRITE_PIPE;
int WORKER_TO_PID[N_WORKERS] = {0};
int WORKER_WRITE_PIPES[N_WORKERS];
int WORKER_READ_PIPES[N_WORKERS];

void initialize_workers() {
    for (int i = 0; i < N_WORKERS; i++) {
        int ends[2];
        pipe(ends);
        WORKER_READ_PIPES[i] = ends[0];
        WORKER_WRITE_PIPES[i] = ends[1];
        fcntl(ends[0], F_SETFL, O_NONBLOCK);
    }
}

int try_read(int fd, void *dst, size_t n_bytes, int block) {
    int total_read = 0;
    do {
        int read_here = read(fd, dst, n_bytes);
        if (read_here != -1) total_read += read_here;
    } while ((total_read || block) && total_read != n_bytes);
    return total_read;
}

int read_worker_bundle(struct message_bundle *bundle, int worker_idx) {
    int fd = WORKER_READ_PIPES[worker_idx];
    if (!try_read(fd, bundle, sizeof(bundle->header), 0))
        return 0;
    size_t message_bytes = sizeof(bundle->messages[0]);
    message_bytes *= bundle->header.n_messages;
    try_read(fd, &(bundle->messages), message_bytes, 1);
    waitpid(WORKER_TO_PID[worker_idx], 0, 0);
    return 1;
}

void send_worker_bundle(struct message_bundle *bundle) {
    if (!WRITE_PIPE) exit(bundle->header.exit_status);
    write(WRITE_PIPE, bundle, sizeof(bundle->header));
    write(WRITE_PIPE, bundle->messages,
          bundle->header.n_messages * sizeof(bundle->messages[0]));
    exit(bundle->header.exit_status);
}

static void sighandler(int which) {
    printf("Got signal: %d\n", which);
    report_error();
}

int spawn_worker(void *data, size_t n_data, int worker_idx) {
    int pid = fork();
    if (!pid) {
        struct sigaction action;
        action.sa_handler = sighandler;
        sigemptyset(&(action.sa_mask));
        action.sa_flags = 0;

        assert(!sigaction(SIGABRT, &action, 0));
        assert(!sigaction(SIGSEGV, &action, 0));

        WRITE_PIPE = WORKER_WRITE_PIPES[worker_idx];
        worker_spawn(data);
        exit(0);
    }
    WORKER_TO_PID[worker_idx] = pid;
    return 1;
}

uint64_t get_time() {
    return time(0);
}
