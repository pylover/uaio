#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "uaio.h"
#include "taskpool.h"


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
    unsigned int maxfileno;
    size_t waitingfiles;
    struct uaio_fileevent *events;
    size_t eventscount;
};


struct uaio {
    struct uaio_taskpool taskpool;
    volatile bool terminating;
#ifdef CONFIG_UAIO_SELECT
    struct uaio_select select;
#endif
};


static struct uaio *_uaio = NULL;


struct uaio_task *
uaio_task_next(struct uaio *c, struct uaio_task *task,
        enum uaio_taskstatus statuses) {
    return uaio_taskpool_next(&c->taskpool, task, statuses);
}


void
uaio_task_killall(struct uaio *c) {
    struct uaio_task *task = NULL;

    while ((task = uaio_taskpool_next(&c->taskpool, task,
                    UAIO_RUNNING | UAIO_WAITING))) {
        task->status = UAIO_TERMINATING;
    }
}


static inline bool
_step(struct uaio_task *task) {
    struct uaio_basecall *call = task->current;

start:
    /* Pre execution */
    if (task->status == UAIO_TERMINATING) {
        /* Tell coroutine to jump to the CORO_FINALLY label */
        call->line = -1;
    }

    call->invoke(task);

    /* Post execution */
    if (task->status == UAIO_TERMINATING) {
        goto start;
    }

    if (task->status == UAIO_TERMINATED) {
        task->current = call->parent;
        free(call);
        if (task->current != NULL) {
            task->status = UAIO_RUNNING;
        }
    }

    return task->current == NULL;
}


struct uaio_task *
uaio_task_new() {
    struct uaio_task *task;

    /* Register task */
    task = uaio_taskpool_lease(&_uaio->taskpool);
    if (task == NULL) {
        return NULL;
    }

    return task;
}


int
uaio_task_dispose(struct uaio_task *task) {
    return uaio_taskpool_release(&_uaio->taskpool, task);
}



#ifdef CONFIG_UAIO_SELECT


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


static int
_select_tick(struct uaio *c, struct uaio_select *s,
        unsigned int timeout_us) {
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


int
uaio_select_monitor(struct uaio_task *task, int fd, int events,
        unsigned int timeout_us) {
    struct uaio_select *s = &_uaio->select;
    struct uaio_fileevent *fe;
    if ((fd < 0) || (fd > s->maxfileno) || (s->eventscount == s->maxfileno)) {
        return -1;
    }

    if (timeout_us > 0) {
        clock_gettime(CLOCK_MONOTONIC, &task->select_timestamp);
        task->select_timeout_us = timeout_us;
    }
    else {
        task->select_timestamp.tv_sec = 0;
        task->select_timestamp.tv_nsec = 0;
        task->select_timeout_us = 0;
    }

    fe = &s->events[s->eventscount++];
    s->waitingfiles++;
    fe->events = events;
    fe->task = task;
    fe->fd = fd;
    return 0;
}


int
uaio_select_forget(int fd) {
    int i;
    struct uaio_fileevent *fe;

    for (i = 0; i < _uaio->select.eventscount; i++) {
        fe = &_uaio->select.events[i];
        if (fe->fd == fd) {
            FILEEVENT_RESET(fe);
            _uaio->select.waitingfiles--;
            return 0;
        }
    }

    return -1;
}


#endif  // CONFIG_UAIO_SELECT


int
uaio_init(size_t maxtasks) {
    _uaio = malloc(sizeof(struct uaio));
    if (_uaio == NULL) {
        return -1;
    }

    _uaio->terminating = false;

    /* Initialize task pool */
    if (uaio_taskpool_init(&_uaio->taskpool, maxtasks)) {
        goto failure;
    }


#ifdef CONFIG_UAIO_SELECT
    /* Select module */
    memset(&_uaio->select, 0, sizeof(struct uaio_select));
    /* select(2) requires the highest number of fileno instead of event count.
     * So, it must increased 3 times for (stdin, stdout and stderr) */
    _uaio->select.maxfileno = CONFIG_UAIO_SELECT_MAXFILES + 3;
    _uaio->select.events = calloc(_uaio->select.maxfileno,
            sizeof(struct uaio_fileevent));
    _uaio->select.eventscount = 0;
    if (_uaio->select.events == NULL) {
        goto failure;
    }

#endif  // CONFIG_UAIO_SELECT

    return 0;

failure:
    uaio_destroy(_uaio);
    return -1;
}


int
uaio_destroy() {
    if (_uaio == NULL) {
        return -1;
    }

    if (_uaio->select.events) {
        free(_uaio->select.events);
    }

    if (uaio_taskpool_deinit(&_uaio->taskpool)) {
        return -1;
    }

    free(_uaio);
    errno = 0;
    return 0;
}
