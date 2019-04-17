#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <regex.h>
#include <fcntl.h>

#include "UDiskEventSocket.h"

#define EVENT_SOCKET_EPOLL_NUM 10
#define UEVENT_BUFFER_SIZE 16 * 1024 * 1024

static void close_socket(PUDISK_SOCKET p_socket) {
    if (NULL == p_socket || -1 == p_socket->hotplug_sock) {
        return;
    }

    close(p_socket->hotplug_sock);
    p_socket->hotplug_sock = -1;
}

PUDISK_SOCKET init_udisk_socket(void)
{
    struct sockaddr_nl snl;
    struct epoll_event ev;
    int ret = -1;

    PUDISK_SOCKET udisk_socket = malloc(sizeof(UDISK_SOCKET));

    udisk_socket->recv_buffer = malloc(UEVENT_BUFFER_SIZE);
    memset(udisk_socket->recv_buffer, 0, UEVENT_BUFFER_SIZE);
    udisk_socket->buffer_size = UEVENT_BUFFER_SIZE;
    
    memset(&snl, 0, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

    udisk_socket->hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (-1 == udisk_socket->hotplug_sock) {
        printf("error getting socket: %s", strerror(errno));
        udisk_socket->buffer_size = 0;
        free(udisk_socket->recv_buffer);
        udisk_socket->recv_buffer = NULL;
        free(udisk_socket);
        udisk_socket = NULL;
        return NULL;
    }

    ret = bind(udisk_socket->hotplug_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));
    if (ret < 0) {
        printf("bind failed: %s", strerror(errno));
        close_socket(udisk_socket);
        udisk_socket->buffer_size = 0;
        free(udisk_socket->recv_buffer);
        udisk_socket->recv_buffer = NULL;
        free(udisk_socket);
        udisk_socket = NULL;
        return NULL;
    }

    udisk_socket->epoll_fd = epoll_create1(0);
    if (udisk_socket->epoll_fd < 0) {
        printf("bind failed: %s", strerror(errno));
        close_socket(udisk_socket);
        udisk_socket->buffer_size = 0;
        free(udisk_socket->recv_buffer);
        udisk_socket->recv_buffer = NULL;
        free(udisk_socket);
        udisk_socket = NULL;
        return NULL;
    }
 
    ev.data.fd = udisk_socket->hotplug_sock;   
    ev.events = EPOLLIN;  
    ret = epoll_ctl(udisk_socket->epoll_fd, EPOLL_CTL_ADD, udisk_socket->hotplug_sock, &ev); 
    if (udisk_socket->epoll_fd < 0) {
        printf("bind failed: %s", strerror(errno));
        close_socket(udisk_socket);
        udisk_socket->buffer_size = 0;
        free(udisk_socket->recv_buffer);
        udisk_socket->recv_buffer = NULL;
        free(udisk_socket);
        udisk_socket = NULL;
        return NULL;
    }

    return udisk_socket;
}

ssize_t recv_event(PUDISK_SOCKET p_socket, int timeout) {
    if (NULL == p_socket) {
        return -1;
    }

    struct epoll_event ret_events[EVENT_SOCKET_EPOLL_NUM];
    int epoll_count = 0;

    epoll_count = epoll_wait(p_socket->epoll_fd, ret_events, EVENT_SOCKET_EPOLL_NUM, timeout);
    if (epoll_count <= 0) {
        return epoll_count;
    }

    int recv_size = 0;
    memset(p_socket->recv_buffer, 0, UEVENT_BUFFER_SIZE);

    for(int i = 0; i < epoll_count; i++) {
        if (ret_events[i].data.fd != p_socket->hotplug_sock || !(ret_events[i].events & EPOLLIN)) {
            return 0;
        }

        int size = recv(p_socket->hotplug_sock, p_socket->recv_buffer + recv_size, UEVENT_BUFFER_SIZE - recv_size, 0);
        if (size < 0) {
            memset(p_socket->recv_buffer, 0, UEVENT_BUFFER_SIZE);
            return -1;
        }

        recv_size += size;
    }
    
    return recv_size;
}

void destroy_udisk_socket(PUDISK_SOCKET p_socket) {
    if (NULL == p_socket) {
        return;
    }

    epoll_ctl(p_socket->epoll_fd, EPOLL_CTL_DEL, p_socket->hotplug_sock, NULL);
    close(p_socket->epoll_fd);
    p_socket->epoll_fd = -1;

    close_socket(p_socket);

    p_socket->buffer_size = 0;

    free(p_socket->recv_buffer);
    p_socket->recv_buffer = NULL;

    free(p_socket);
    p_socket = NULL;
}
