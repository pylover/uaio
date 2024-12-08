#include <time.h>
#include <errno.h>

#include <elog.h>

#include "uaio.h"
#include "select.h"


#define TSEMPTY(ts) (!((ts).tv_sec || (ts).tv_nsec))


static long
timediff(struct timespec start, struct timespec end) {
    long sec;
    long nsec;

    if ((end.tv_nsec - start.tv_nsec) < 0) {
        sec = end.tv_sec - start.tv_sec - 1;
        nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else {
        sec = end.tv_sec - start.tv_sec;
        nsec = end.tv_nsec - start.tv_nsec;
    }
    return (sec * 1000000) + (nsec / 1000);
}


static long
_select_task_timeout_us(struct uaio_task *task) {
    struct timespec now;
    long diff_us;

    if (TSEMPTY(task->select_timestamp)) {
        return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    diff_us = timediff(task->select_timestamp, now);
    return task->select_timeout_us - diff_us;
}


int
uaio_select_tick(struct uaio_select *s, unsigned int timeout_us) {
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

    errno = 0;
    nfds = select(s->maxfileno + 1, &rfds, &wfds, &efds, &tv);
    if (nfds == -1) {
        return -1;
    }

    // if (nfds == 0) {
    //     return 0;
    // }

    shift = 0;
    int ttout;
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

        if ((fe->task->status == UAIO_WAITING) &&
            ((ttout = _select_task_timeout_us(fe->task)) < 0)) {
            fe->task->status = UAIO_RUNNING;
            fe->task->select_timeout_us = ttout;
            s->waitingfiles--;
            FILEEVENT_RESET(fe);
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
