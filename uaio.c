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

#include "clog/clog.h"
#include "uaio/uaio.h"
#include "uaio/taskpool.h"


static struct uaio_taskpool _tasks;


int
uaio_init(size_t maxtasks) {
    /* Initialize task pool */
    if (taskpool_init(&_tasks, maxtasks)) {
        goto onerror;
    }

    return 0;

onerror:
    uaio_deinit();
    return -1;
}


void
uaio_deinit() {
    taskpool_deinit(&_tasks);
    errno = 0;
}


void
uaio_task_dispose(struct uaio_task *task) {
    taskpool_delete(&_tasks, task->index);
    free(task);
}


struct uaio_task *
uaio_task_new() {
    int index;
    struct uaio_task *task;

    if (TASKPOOL_ISFULL(&_tasks)) {
        return NULL;
    }

    task = malloc(sizeof(struct uaio_task));
    if (task == NULL) {
        return NULL;
    }

    /* Register task */
    index = taskpool_append(&_tasks, task);
    if (index == -1) {
        free(task);
        return NULL;
    }
    task->index = index;
    task->eno = 0;
    task->current = NULL;

    return task;
}


void
uaio_task_killall() {
    int taskindex;
    struct uaio_task *task;

    for (taskindex = 0; taskindex < _tasks.count; taskindex++) {
        task = taskpool_get(&_tasks, taskindex);
        if (task == NULL) {
            continue;
        }

        task->status = UAIO_TERMINATING;
    }
}


static void
uaio_invoker_default(struct uaio_task *task) {
    struct uaio_call *call = task->current;

    call->coro(task, call->state);
}


int
uaio_call_new(struct uaio_task *task, uaio_coro coro, void *state) {
    struct uaio_call *call = malloc(sizeof(struct uaio_call));
    if (call == NULL) {
        return -1;
    }

    call->parent = task->current;
    call->coro = coro;
    call->state = state;
    call->line = 0;
    call->invoke = uaio_invoker_default;

    task->status = UAIO_RUNNING;
    task->current = call;
    return 0;
}


bool
uaio_task_step(struct uaio_task *task) {
    struct uaio_call *call = task->current;

start:

    /* Pre execution */
    switch (task->status) {
        case UAIO_TERMINATING:
            /* Tell coroutine to jump to the CORO_FINALLY label */
            call->line = -1;
            break;

        case UAIO_SLEEPING:
            /* Ignore if task is waiting for IO events */
            return false;

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

    if (task->current == NULL) {
        uaio_task_dispose(task);
        return true;
    }

    return false;
}


int
uaio_loop() {
    int taskindex;
    struct uaio_task *task = NULL;
    bool vacuum_needed;

    while (_tasks.count) {
        vacuum_needed = false;

        for (taskindex = 0; taskindex < _tasks.count; taskindex++) {
            task = taskpool_get(&_tasks, taskindex);
            if (task == NULL) {
                continue;
            }

            vacuum_needed |= uaio_task_step(task);
        }

        if (vacuum_needed) {
            taskpool_vacuum(&_tasks);
        }
    }

    return 0;
}


int
uaio_spawn(uaio_coro coro, void *state) {
    struct uaio_task *task = NULL;

    task = uaio_task_new();
    if (task == NULL) {
        return -1;
    }

    if (uaio_call_new(task, coro, state)) {
        goto failure;
    }

    return 0;

failure:
    uaio_task_dispose(task);
    return -1;
}


int
uaio_handover() {
    if (uaio_loop()) {
        goto onerror;
    }

    uaio_deinit();
    return 0;

onerror:
    uaio_deinit();
    return -1;
}


int
uaio_forever(uaio_coro coro, void *state, size_t maxtasks) {
    if (uaio_init(maxtasks)) {
        return -1;
    }

    if (uaio_spawn(coro, state)) {
        goto failure;
    }

    if (uaio_loop()) {
        goto failure;
    }

    uaio_deinit();
    return 0;

failure:
    uaio_deinit();
    return -1;
}
