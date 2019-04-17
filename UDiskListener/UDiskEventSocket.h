#ifndef U_DISK_EVENT_SOCKET_H
#define U_DISK_EVENT_SOCKET_H

#include <pthread.h>
#include <sys/epoll.h>

typedef struct {
    int hotplug_sock;
    char *recv_buffer;
    int buffer_size;
    int epoll_fd;
} UDISK_SOCKET, *PUDISK_SOCKET;

PUDISK_SOCKET init_udisk_socket(void);
void destroy_udisk_socket(PUDISK_SOCKET p_socket);
ssize_t recv_event(PUDISK_SOCKET p_socket, int timeout);

#endif