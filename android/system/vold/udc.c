/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
* sync from gs702c_4.4.4
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/queue.h>
#include <linux/netlink.h>

#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>


static void usage(char *progname);
static int do_monitor(int sock, int stop_after_cmd, char *subsys);
static int do_cmd(int sock, int argc, char **argv);
static int uevent_open();
static int uevent_read(int fd, char* buf, int len);

int main(int argc, char **argv) {
    int sock;

    if (argc < 2)
        usage(argv[0]);

    if ((sock = uevent_open()) < 0) {
        fprintf(stderr, "Error connecting (%s)\n", strerror(errno));
        exit(4);
    }

    if (!strcmp(argv[1], "monitor"))
        exit(do_monitor(sock, 0, argv[2]));
    exit(do_cmd(sock, argc, argv));
}

static int do_cmd(int sock, int argc, char **argv) {
    char final_cmd[4096] = { '\0' };
    int i;
    int ret;
    int fd;

    for (i = 1; i < argc; i++) {
        char *cmp;

        if (!index(argv[i], ' '))
            asprintf(&cmp, "%s%s", argv[i], (i == (argc -1)) ? "" : "\n");
        else
            asprintf(&cmp, "\"%s\"%s", argv[i], (i == (argc -1)) ? "" : "\n");

        ret = strlcat(final_cmd, cmp, sizeof(final_cmd));
        if (ret >= sizeof(final_cmd))
            abort();
        free(cmp);
    }

    if ((fd = open("/sys/kernel/uevent_fake", O_WRONLY)) < 0) {
        perror("open");
        return errno;
    } else {
        if (write(fd, final_cmd, strlen(final_cmd) + 1) < 0) {
            perror("write");
            close(fd);
            return errno;
        }
        close(fd);
    }

    return do_monitor(sock, 1, NULL);
}

static int do_monitor(int sock, int stop_after_cmd, char *subsys) {
    char *buffer = malloc(4096);

    if (!stop_after_cmd)
        printf("[Connected to Uevent]\n");

    printf("\n");
    
    while(1) {
        int rc = 0;
        int i = 0;
        int offset = 0;
        
        rc = uevent_read(sock, buffer, 4096);        

        if (subsys != NULL) {
            // get subsystem            
            char *p = buffer;
            char *sub = NULL;            
            while(*p) {
                if (!strncmp(p, "SUBSYSTEM=", 10)) {
                    p += 10;
                    sub = p;
                }
                while(*p++);
            }
            
            // ignore other subsystem
            if (strcmp(sub, subsys) != 0) {
                continue;
            }
        }
        
        for (i = 0; i < rc; i++) {
            if (buffer[i] == '\0') {
                printf("%s\n", buffer + offset);
                offset = i + 1;
            }
        }
        
        printf("\n");
        
        if (stop_after_cmd)
            break;
    }
    free(buffer);
    return 0;
}

static void usage(char *progname) {
    fprintf(stderr, "Usage: %s <monitor><subsystem>|<cmd> [arg1] [arg2...]\n", progname);
    exit(1);
}

static int uevent_open()
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    return s;
}

static int uevent_read(int fd, char* buf, int len)
{
    while (1) {
        struct pollfd fds;
        int nr;
    
        fds.fd = fd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);
     
        if(nr > 0 && fds.revents == POLLIN) {
            int count = recv(fd, buf, len, 0);
            if (count > 0) {
                return count;
            } 
        }
    }
    
    // won't get here
    return 0;
}
