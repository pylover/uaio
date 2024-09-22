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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "uaio/uaio.h"
#include "uaio/taskpool.h"


struct uaio {
    struct uaio_taskpool taskpool;
    volatile bool terminating;
#ifdef UAIO_MODULES
    struct uaio_module *modules[UAIO_MODULES_MAX];
    size_t modulescount;
#endif  // UAIO_MODULES
};


struct uaio*
uaio_create(size_t maxtasks) {
    struct uaio *c = malloc(sizeof(struct uaio));
    if (c == NULL) {
        return NULL;
    }

    c->terminating = false;

#ifdef UAIO_MODULES
    c->modulescount = 0;
#endif  // UAIO_MODULES

    /* Initialize task pool */
    if (uaio_taskpool_init(&c->taskpool, maxtasks)) {
        goto onerror;
    }

    return c;

onerror:
    uaio_destroy(c);
    return NULL;
}


int
uaio_destroy(struct uaio *c) {
    if (c == NULL) {
        return -1;
    }

    if (uaio_taskpool_deinit(&c->taskpool)) {
        return -1;
    }

    free(c);
    errno = 0;
    return 0;
}


struct uaio_task *
uaio_task_new(struct uaio *c) {
    struct uaio_task *task;

    /* Register task */
    task = uaio_taskpool_lease(&c->taskpool);
    if (task == NULL) {
        return NULL;
    }

    task->uaio = c;
    return task;
}


int
uaio_task_dispose(struct uaio_task *task) {
    return uaio_taskpool_release(&(task->uaio->taskpool), task);
}


void
uaio_task_killall(struct uaio *c) {
    struct uaio_task *task = NULL;

    while ((task = uaio_taskpool_next(&c->taskpool, task,
                    UAIO_RUNNING | UAIO_WAITING))) {
        task->status = UAIO_TERMINATING;
    }
}


#ifdef UAIO_MODULES

int
uaio_module_install(struct uaio *c, struct uaio_module *m) {
    if (c->modulescount == UAIO_MODULES_MAX) {
        return -1;
    }

    if ((c == NULL) || (m == NULL)) {
        return -1;
    }

    c->modules[c->modulescount++] = m;
    return 0;
}


int
uaio_module_uninstall(struct uaio *c, struct uaio_module *m) {
    if (c->modulescount == 0) {
        return -1;
    }

    if ((c == NULL) || (m == NULL)) {
        return -1;
    }

    int i;
    int shift = 0;

    for (i = 0; i < c->modulescount; i++) {
        if (c->modules[i] == m) {
            shift++;
            continue;
        }

        if (!shift) {
            continue;
        }

        c->modules[i - shift] = c->modules[i];
        c->modules[i] = NULL;
    }

    c->modulescount -= shift;
    return 0;
}


#endif  // UAIO_MODULES


static inline bool
_step(struct uaio_task *task) {
    struct uaio_basecall *call = task->current;

start:
    /* Pre execution */
    switch (task->status) {
        case UAIO_TERMINATING:
            /* Tell coroutine to jump to the CORO_FINALLY label */
            call->line = -1;
            break;
        default:
    }

    call->invoke(task);

    /* Post execution */
    switch (task->status) {
        case UAIO_TERMINATING:
            goto start;
        case UAIO_TERMINATED:
            task->current = call->parent;
            free(call);
            if (task->current != NULL) {
                task->status = UAIO_RUNNING;
            }
            break;
        default:
    }

    return task->current == NULL;
}


int
uaio_loop(struct uaio *c) {
    struct uaio_task *task = NULL;
    struct uaio_taskpool *taskpool = &c->taskpool;

#ifdef UAIO_MODULES
    int i;
    unsigned int modtimeout = 1000;
    struct uaio_module *module;

    for (i = 0; i < c->modulescount; i++) {
        module = c->modules[i];
        if (module->loopstart && module->loopstart(c, module)) {
            return -1;
        }
    }

loop:
#endif

    while (taskpool->count) {

#ifdef UAIO_MODULES
        if (!c->terminating) {
            for (i = 0; i < c->modulescount; i++) {
                module = c->modules[i];
                if (module->tick && module->tick(c, module, modtimeout)) {
                    goto interrupt;
                }
            }
        }
#endif

        task = uaio_taskpool_next(taskpool, task,
                    UAIO_RUNNING | UAIO_TERMINATING);
        if (task == NULL) {
#ifdef UAIO_MODULES
            modtimeout = UAIO_MODULES_TICKTIMEOUT_LONG_US / c->modulescount;
#endif
            continue;
        }

        do {
            if (_step(task)) {
                uaio_taskpool_release(taskpool, task);
            }
        } while ((task = uaio_taskpool_next(taskpool, task,
                    UAIO_RUNNING | UAIO_TERMINATING)));
#ifdef UAIO_MODULES
        modtimeout = UAIO_MODULES_TICKTIMEOUT_SHORT_US;
#endif
    }

#ifdef UAIO_MODULES
    for (i = 0; i < c->modulescount; i++) {
        module = c->modules[i];
        if (module->loopend && module->loopend(c, module)) {
            return -1;
        }
    }
#endif

    return 0;

#ifdef UAIO_MODULES
interrupt:
    c->terminating = true;
    uaio_task_killall(c);
    goto loop;
#endif
}
