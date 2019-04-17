#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "includes/UDiskListener.h"

static int running = 1;

static void sig_int(int sig) {
    running = 0;
}

int main(void) {
    PUDISK_LISTENER listener = init_udisk_listener();
    int ret = start(listener);
    if (ret < 0) {
        exit(1);
    }
    
    signal(SIGINT, sig_int);

    while (running) {
        sleep(1);
        continue;
    }

    stop(listener);
    destroy_udisk_listener(listener);

    exit(0);
}
