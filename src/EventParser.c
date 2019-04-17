#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#include "../includes/EventParser.h"
#include "../includes/UDiskEventClient.h"

#define UDISK_PATTERN "(\\w+)@.*?/block/(s\\w{2})/(s\\w{3})"
#define PORC_MOUNTS_DIR "/proc/mounts"
#define DEV_DIR "/dev/"
#define ENENT_TYPE_ADD "add"
#define ENENT_TYPE_REMOVE "remove"
#define UN_SERVER_ADDR "/var/udisk_listener.sock"

#define ADD_EVENT_PARSE_COUNT 5
#define ADD_EVENT_PARSE_SLEEP_SEC 2

typedef struct {
    char *buffer;
    int size;
} EVENT_ARGS, *PEVENT_ARGS;

static PEVENT_ARGS init_event_args(char *event, int size) {
    PEVENT_ARGS args = malloc(sizeof(EVENT_ARGS));
    args->buffer = malloc(size);
    args->size = size;

    memset(args->buffer, 0, size);
    memcpy(args->buffer, event, size);

    return args;
}

static void destroy_event_args(PEVENT_ARGS args) {
    if (NULL != args->buffer) {
        free(args->buffer);
        args->buffer = NULL;
    }

    if (NULL != args) {
        free(args);
        args = NULL;
    }
}

static void *parse_thread(void *arg) {
    if (NULL == arg) {
        return NULL;
    }

    PEVENT_ARGS param = (PEVENT_ARGS)arg;

    EVENT_ARGS event;
    memcpy(&event, param, sizeof(EVENT_ARGS));

    destroy_event_args(param);

    int err = 0;
    regex_t reg;
    int nm = 4;
    regmatch_t pmatch[nm];

    // 正则匹配，查找事件类型和设备文件路径
    if (regcomp(&reg, UDISK_PATTERN, REG_EXTENDED) < 0) {
        return NULL;
    }

    err = regexec(&reg, event.buffer, nm, pmatch, 0);
    if (0 != err) {
        regfree(&reg);
        return NULL;
    }

    char type[100];
    if (-1 != pmatch[1].rm_so) {
        int len = pmatch[1].rm_eo - pmatch[1].rm_so;
        if (len < 0) {
            regfree(&reg);
            return NULL;
        }
        
        memset(type, 0, sizeof(type));
        memcpy(type, event.buffer + pmatch[1].rm_so, len);
    }

    char dev_path[100];
    if (-1 != pmatch[3].rm_so) {
        int len = pmatch[3].rm_eo - pmatch[3].rm_so;
        if (len < 0) {
            regfree(&reg);
            return NULL;
        }

        memset(dev_path, 0, sizeof(dev_path));
        strcpy(dev_path, DEV_DIR);
        memcpy(dev_path + strlen(DEV_DIR), event.buffer + pmatch[3].rm_so, len);
    }

    regfree(&reg);

    // 获取挂载路径
    char mnt_path[100];
    memset(mnt_path, 0, sizeof(mnt_path));
    if (!strcmp(type, ENENT_TYPE_ADD)) {
        int count = ADD_EVENT_PARSE_COUNT;
        int sec = ADD_EVENT_PARSE_SLEEP_SEC;

        while (count--) {
            sleep(sec);

            FILE *file = fopen(PORC_MOUNTS_DIR, "r");
            if (NULL == file) {
                continue;
            }

            while(!feof(file)) {
                char line[1024];
                memset(line, 0, 1024);
                fgets(line, sizeof(line) - 1, file); 

                char *dev_start = strstr(line, dev_path);
                if (NULL == dev_start) {
                    continue;
                }

                char *mount_start = line + strlen(dev_path) + 1;
                char *mount_end = strstr(mount_start, " ");
                memcpy(mnt_path, mount_start, mount_end - mount_start);
                break;
            }  
        
            fclose(file);
            file = NULL;
        }
    }

    char result[256];
    memset(result, 0, 256);

    strcat(result, type);
    strcat(result, " ");
    strcat(result, dev_path);
    strcat(result, " ");
    strcat(result, mnt_path);
    strcat(result, " ");

    err = send_un(result, strlen(result), UN_SERVER_ADDR);
    if (err < 0) {
        printf("send: %s failed\n", result);
        return NULL;
    }

    printf("send: %s succeed\n", result);

    return NULL;
}

void create_parse_detach_thread(char *event, int size) {
    if (NULL == event || 0 == size) {
        return;
    }

    pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t thread_id;
	pthread_create(&thread_id, &attr, parse_thread, init_event_args(event, size));

    pthread_attr_destroy(&attr);
}