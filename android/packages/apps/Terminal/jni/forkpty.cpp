/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "forkpty"

#include <utils/Log.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "forkpty.h"

pid_t forkpty(int *master, int *slave, const struct termios *termp,
        const struct winsize *winp) {
    int ptm = open("/dev/ptmx", O_RDWR);
    if (ptm < 0) {
        ALOGE("cannot open /dev/ptmx - %s", strerror(errno));
        return -1;
    }
    fcntl(ptm, F_SETFD, FD_CLOEXEC);

    char *devname;
    if (grantpt(ptm) || unlockpt(ptm) || ((devname = (char *) ptsname(ptm)) == 0)) {
        ALOGE("error opening pty - %s", strerror(errno));
        return -1;
    }

    int pts = open(devname, O_RDWR);
    if (pts < 0) {
        ALOGE("unable to open slave pty - %s", strerror(errno));
        return -1;
    }

    if (termp) {
        tcsetattr(pts, TCSAFLUSH, termp);
    }

    if (winp) {
        ioctl(pts, TIOCSWINSZ, winp);
    }

    pid_t pid = fork();

    if (pid < 0) {
        ALOGE("fork failed - %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        setsid();
        if (ioctl(pts, TIOCSCTTY, (char *)NULL) == -1) exit(-1);
        dup2(pts, STDIN_FILENO);
        dup2(pts, STDOUT_FILENO);
        dup2(pts, STDERR_FILENO);
        if (pts > 2) {
            close(pts);
        }
    } else {
        *master = ptm;
        if (slave) {
            *slave = pts;
        }
    }
    return pid;
}

