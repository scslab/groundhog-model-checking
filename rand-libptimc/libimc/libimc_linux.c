#include "_libimc.h"
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>

extern void launch_replay(choice_t *path);
