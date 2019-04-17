#ifndef U_DISK_LISTENER_H
#define U_DISK_LISTENER_H

#include <pthread.h>

#include "UDiskEventSocket.h"

typedef struct {
    PUDISK_SOCKET event_socket;
    pthread_t listen_thread;
    int start;
} UDISK_LISTENER, *PUDISK_LISTENER;

PUDISK_LISTENER init_udisk_listener();
void destroy_udisk_listener(PUDISK_LISTENER p_listener);
int start(PUDISK_LISTENER p_listener);
void stop(PUDISK_LISTENER p_listener);

#endif