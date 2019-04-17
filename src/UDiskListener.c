#include <stdio.h>
#include <stdlib.h>

#include "../includes/UDiskListener.h"
#include "../includes/EventParser.h"

#define EVENT_RECV_TIMEOUT_MS 1000

static void *recv_event_thread(void *args) {
    if (NULL == args) {
        return NULL;
    }

    PUDISK_LISTENER p_listener = (PUDISK_LISTENER)args;
    while (p_listener->start) {
        if (NULL == p_listener->event_socket) {
            p_listener->event_socket = init_udisk_socket();
            if (NULL == p_listener->event_socket) {
                continue;
            }
        }

        int size = recv_event(p_listener->event_socket, EVENT_RECV_TIMEOUT_MS);
        if (0 == size) {
            continue;
        } else if (size < 0) {
            destroy_udisk_socket(p_listener->event_socket);
            continue;
        }

        create_parse_detach_thread(p_listener->event_socket->recv_buffer, p_listener->event_socket->buffer_size);
    }

    return NULL;
}

PUDISK_LISTENER init_udisk_listener() {
    PUDISK_LISTENER listener = malloc(sizeof(UDISK_LISTENER));

    listener->start = 0;

    return listener;
}

void destroy_udisk_listener(PUDISK_LISTENER p_listener) {
    if (NULL == p_listener) {
        return;
    }

    p_listener->start = 0;

    free(p_listener);
    p_listener = NULL;
}

int start(PUDISK_LISTENER p_listener) {
    int ret = -1;
    if (NULL == p_listener) {
        return ret;
    }

    p_listener->event_socket = init_udisk_socket();
    if (NULL == p_listener->event_socket) {
        printf("init event_socket error");
        return ret;
    }

    p_listener->start = 1;

    return pthread_create(&p_listener->listen_thread, NULL, recv_event_thread, p_listener);
}

void stop(PUDISK_LISTENER p_listener) {
    if (NULL == p_listener) {
        return;
    }

    p_listener->start = 0;

    destroy_udisk_socket(p_listener->event_socket);

    pthread_join(p_listener->listen_thread, NULL);
}
