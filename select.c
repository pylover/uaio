// Copyright 2023 Vahid Mardani
/*
 * This file is part of uaio.
 *  uaio is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  uaio is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with uaio. If not, see <https://www.gnu.org/licenses/>.
 *
 *  Author: Vahid Mardani <vahid.mardani@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#ifndef UAIO_FDMON_MAXFILES
#include <sys/resource.h>
#endif

#include "uaio/select.h"


#define FILEEVENT_RESET(fe) \
            (fe)->task = NULL; \
            (fe)->fd = -1; \
            (fe)->events = 0


struct uaio_fileevent {
    int fd;
    int events;
    struct uaio_task *task;
};


struct uaio_select {
    struct uaio_fdmon;
    unsigned int maxfileno;
    size_t waitingfiles;
    struct uaio_fileevent *events;
    size_t eventscount;
};


static int
_tick(struct uaio *c, struct uaio_select *s, unsigned int timeout_us) {
    int i;
    int fd;
    int nfds;
    int shift;
    struct uaio_fileevent *fe;
    struct timeval tv;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;

    if (s->waitingfiles == 0) {
        return 0;
    }

    tv.tv_usec = timeout_us % 1000000;
    tv.tv_sec = timeout_us / 1000000;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        fd = fe->fd;

        if (fe->events == 0) {
            s->waitingfiles--;
            continue;
        }

        if (fe->events & UAIO_IN) {
            FD_SET(fd, &rfds);
        }

        if (fe->events & UAIO_OUT) {
            FD_SET(fd, &wfds);
        }

        if (fe->events & UAIO_ERR) {
            FD_SET(fd, &efds);
        }
    }

    nfds = select(s->maxfileno + 1, &rfds, &wfds, &efds, &tv);
    if (nfds == -1) {
        return -1;
    }

    if (nfds == 0) {
        return 0;
    }

    shift = 0;
    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        fd = fe->fd;

        if ((fd == -1)
                || FD_ISSET(fd, &rfds)
                || FD_ISSET(fd, &wfds)
                || FD_ISSET(fd, &efds)) {
            if (fe->task && (fe->task->status == UAIO_WAITING)) {
                fe->task->status = UAIO_RUNNING;
                s->waitingfiles--;
                FILEEVENT_RESET(fe);
            }
            shift++;
            continue;
        }

        if (!shift) {
            continue;
        }

        s->events[i - shift] = *fe;
        FILEEVENT_RESET(fe);
    }
    s->eventscount = s->waitingfiles;
    return 0;
}


static int
_monitor(struct uaio_select *s, struct uaio_task *task, int fd, int events) {
    struct uaio_fileevent *fe;
    if ((fd < 0) || (fd > s->maxfileno) || (s->eventscount == s->maxfileno)) {
        return -1;
    }

    fe = &s->events[s->eventscount++];
    s->waitingfiles++;
    fe->events = events;
    fe->task = task;
    fe->fd = fd;
    return 0;
}


static int
_forget(struct uaio_select *s, int fd) {
    int i;
    struct uaio_fileevent *fe;

    for (i = 0; i < s->eventscount; i++) {
        fe = &s->events[i];
        if (fe->fd == fd) {
            FILEEVENT_RESET(fe);
            s->waitingfiles--;
            return 0;
        }
    }

    return -1;
}


struct uaio_select *
uaio_select_create(struct uaio* c, size_t maxevents) {
    struct uaio_select *s;

    /* Create select instance */
    s = malloc(sizeof(struct uaio_select));
    if (s == NULL) {
        return NULL;
    }
    memset(s, 0, sizeof(struct uaio_select));

    s->waitingfiles = 0;
    s->tick = (uaio_tick) _tick;
    s->monitor = (uaio_filemonitor)_monitor;
    s->forget = (uaio_fileforget)_forget;

    if (uaio_module_install(c, (struct uaio_module*)s)) {
        goto failed;
    }

#ifndef UAIO_FDMON_MAXFILES
    /* Find maximum allowed file descriptors for this process and allocate
     * as much as needed for task repository
     */
    struct rlimit limits;
    if (getrlimit(RLIMIT_NOFILE, &limits)) {
        goto failed;
    }

    if (maxevents > limits.rlim_max) {
        goto failed;
    }
#else
    if (maxevents > UAIO_FDMON_MAXFILES) {
        goto failed;
    }
#endif
    /* select(2) requires the highest number of fileno instead of event count.
     * So, it must increased 3 times for (stdin, stdout and stderr) */
    s->maxfileno = maxevents + 3;
    s->events = calloc(s->maxfileno, sizeof(struct uaio_fileevent));
    s->eventscount = 0;
    if (s->events == NULL) {
        goto failed;
    }
    return s;

failed:
    free(s);
    return NULL;
}


int
uaio_select_destroy(struct uaio* c, struct uaio_select *s) {
    int ret = 0;

    if (s == NULL) {
        return -1;
    }

    ret |= uaio_module_uninstall(c, (struct uaio_module*)s);

    if (s->events) {
        free(s->events);
    }

    free(s);
    return 0;
}
