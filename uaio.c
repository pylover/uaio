// #include <errno.h>
// #include <stdlib.h>
// #include <string.h>

#include "uaio.h"
#include "taskpool.h"


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
