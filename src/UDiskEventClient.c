#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "../includes/UDiskEventClient.h"

int send_un(char *buf, int size, char *addr) {
    if (NULL == buf || 0 == size || NULL == addr) {
        return -1;
    }

    int sockfd = -1;
    int confd = -1;
    int ret = -1;
    struct sockaddr_un serveraddr, clientaddr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("socket create error: %s\n", strerror(errno));
        return -1;
    }

    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, addr);

    ret = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret < 0) {
        printf("socket connect error: %s\n", strerror(errno));
        return -1;
    }

    ret = send(sockfd, buf, size, 0);
    if (ret < 0) {
        printf("socket send error: %s\n", strerror(errno));
        return -1;
    }

    close(sockfd);
    sockfd = -1;

    return 0;
}