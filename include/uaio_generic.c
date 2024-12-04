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
#include <stdlib.h>  // NOLINT
#include <unistd.h>

#include "uaio/uaio.h"
#include "uaio/semaphore.h"


void
UAIO_NAME(invoker)(struct uaio_task *task) {
    struct UAIO_NAME(call) *call = (struct UAIO_NAME(call)*) task->current;

    call->coro(task, call->state
#ifdef UAIO_ARG1
        , call->arg1
    #ifdef UAIO_ARG2
            , call->arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
    );  // NOLINT
}


int
UAIO_NAME(call_new)(struct uaio_task *task, UAIO_NAME(coro) coro,
        UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        ) {
    struct UAIO_NAME(call) *call;

    call = malloc(sizeof(struct UAIO_NAME(call)));
    if (call == NULL) {
        return -1;
    }

    call->parent = task->current;
    call->coro = coro;
    call->state = state;
    call->line = 0;
    call->invoke = UAIO_NAME(invoker);

    task->status = UAIO_RUNNING;
    task->current = (struct uaio_basecall*) call;

    /* arguments */
#ifdef UAIO_ARG1
    call->arg1 = arg1;
    #ifdef UAIO_ARG2
        call->arg2 = arg2;
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
    return 0;
}


int
UAIO_NAME(spawn) (struct uaio *c, UAIO_NAME(coro) coro, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        ) {
    struct uaio_task *task = NULL;

    task = uaio_task_new(c);
    if (task == NULL) {
        return -1;
    }

    if (UAIO_NAME(call_new)(task, coro, state
#ifdef UAIO_ARG1
        , arg1
    #ifdef UAIO_ARG2
            , arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        )) {  // NOLINT
        goto failure;
    }

    return 0;

failure:
    uaio_task_dispose(task);
    return -1;
}


int
UAIO_NAME(forever) (UAIO_NAME(coro) coro, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        , size_t maxtasks) {
    struct uaio * c = uaio_create(maxtasks);
    if (c == NULL) {
        return -1;
    }

    if (UAIO_NAME(spawn)(c, coro, state
#ifdef UAIO_ARG1
        , arg1
    #ifdef UAIO_ARG2
            , arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        )) {  // NOLINT
        goto failure;
    }

    if (uaio_loop(c)) {
        goto failure;
    }

    uaio_destroy(c);
    return 0;

failure:
    uaio_destroy(c);
    return -1;
}


#ifdef CONFIG_UAIO_SEMAPHORE


int
UAIO_NAME(spawn_semaphore) (struct uaio *c, struct uaio_semaphore *semaphore,
        UAIO_NAME(coro) coro, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        ) {
    struct uaio_task *task = NULL;

    task = uaio_task_new(c);
    if (task == NULL) {
        return -1;
    }

    task->semaphore = semaphore;

    if (UAIO_NAME(call_new)(task, coro, state
#ifdef UAIO_ARG1
        , arg1
    #ifdef UAIO_ARG2
            , arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        )) {  // NOLINT
        goto failure;
    }

    uaio_semaphore_acquire(task);
    return 0;

failure:
    uaio_task_dispose(task);
    return -1;
}


#endif
