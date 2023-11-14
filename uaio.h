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
#ifndef UAIO_UAIO_H_
#define UAIO_UAIO_H_


#include <errno.h>

#include "clog/clog.h"


/* Generic stuff */
#define UAIO_NAME_PASTER(x, y) x ## _ ## y
#define UAIO_NAME_EVALUATOR(x, y)  UAIO_NAME_PASTER(x, y)
#define UAIO_NAME(n) UAIO_NAME_EVALUATOR(UAIO_ENTITY, n)
#define ASYNC void
#define AWAIT(entity, coro, ...) \
    do { \
        (self)->current->line = __LINE__; \
        if (entity ## _call_new(self, coro, __VA_ARGS__)) { \
            (self)->status = UAIO_TERMINATING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)
#define UAIO_AWAIT(coro, ...) AWAIT(uaio, (uaio_coro)coro, __VA_ARGS__)
#define UAIO_IWAIT(e) \
    do { \
        (self)->current->line = __LINE__; \
        (self)->status = UAIO_SLEEPING; \
        e; \
        return; \
        case __LINE__:; \
    } while (0)


#define CORO_START \
    switch ((self)->current->line) { \
        case 0:


#define CORO_FINALLY \
        case -1:; } \
    (self)->status = UAIO_TERMINATED


#define CORO_RETURN \
    (self)->status = UAIO_TERMINATING; \
    return


#define CORO_REJECT(n) \
    (self)->eno = n; \
    (self)->status = UAIO_TERMINATING; \
    return


#define UAIO_HASERROR(task) (task->eno != 0)
#define UAIO_ISERROR(task, e) (UAIO_HASERROR(task) && (task->eno == e))
#define UAIO_CLEARERROR(task) task->eno = 0


#define CORO_MUSTWAITFD() \
    ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))


#define UAIO_SPAWN(coro, state) uaio_spawn((uaio_coro)(coro), (void*)(state))
#define UAIO_FOREVER(coro, state, maxtasks) \
    uaio_forever((uaio_coro)(coro), (void*)(state), maxtasks)


enum uaio_taskstatus {
    UAIO_RUNNING,
    UAIO_SLEEPING,
    UAIO_TERMINATING,
    UAIO_TERMINATED,
};


struct uaio_task;
typedef void (*uaio_coro) (struct uaio_task *self, void *state);
typedef void (*uaio_invoker) (struct uaio_task *self);


struct uaio_call {
    struct uaio_call *parent;
    int line;
    uaio_coro coro;
    void *state;
    uaio_invoker invoke;
};


struct uaio_task {
    int index;
    int eno;
    enum uaio_taskstatus status;
    struct uaio_call *current;
};


struct uaio_taskpool {
    struct uaio_task **pool;
    size_t size;
    size_t count;
};


int
uaio_forever(uaio_coro coro, void *state, size_t maxtasks);


int
uaio_handover();


int
uaio_init(size_t maxtasks);


void
uaio_deinit();


int
uaio_loop();


struct uaio_task *
uaio_task_new();


int
uaio_call_new(struct uaio_task *task, uaio_coro coro, void *state);


void
uaio_task_dispose(struct uaio_task *task);


void
uaio_task_killall();


int
uaio_spawn(uaio_coro coro, void *state);


#endif  // UAIO_UAIO_H_
