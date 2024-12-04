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
#ifndef UAIO_H_
#define UAIO_H_


#include <stddef.h>

#include "esp_timer.h"

#ifdef CONFIG_UAIO_SELECT
#include <time.h>
#endif


/* Generic stuff */
#define UAIO_NAME_PASTER(x, y) x ## _ ## y
#define UAIO_NAME_EVALUATOR(x, y)  UAIO_NAME_PASTER(x, y)
#define UAIO_NAME(n) UAIO_NAME_EVALUATOR(UAIO_ENTITY, n)


enum uaio_taskstatus {
    UAIO_IDLE = 1,
    UAIO_RUNNING = 2,
    UAIO_WAITING = 4,
    UAIO_TERMINATING = 8,
    UAIO_TERMINATED = 16,
};


struct uaio;
struct uaio_task;


/* using sleep amount in microseconds as state */
typedef unsigned long uaio_sleep_t;
typedef void (*uaio_invoker) (struct uaio_task *self);


struct uaio_basecall {
    struct uaio_basecall *parent;
    int line;
    uaio_invoker invoke;
};


#ifdef CONFIG_UAIO_SEMAPHORE
    struct uaio_semaphore;
#endif


struct uaio_task {
    struct uaio_basecall *current;
    enum uaio_taskstatus status;
    int eno;

#ifdef CONFIG_UAIO_SELECT
    struct timespec select_timestamp;
    long select_timeout_us;
#endif

#ifdef CONFIG_UAIO_SEMAPHORE
    struct uaio_semaphore *semaphore;
#endif

    /* esp32 idf */
    esp_timer_handle_t sleep;
};


int
uaio_init(size_t maxtasks);


int
uaio_destroy();


struct uaio_task *
uaio_task_new();


int
uaio_task_dispose(struct uaio_task *task);


int
uaio_loop();


void
uaio_task_killall();


void
uaio_task_sleep(struct uaio_task *task, unsigned long us);


#define UAIO_BEGIN(task) \
    switch ((task)->current->line) { \
        case 0:


#define UAIO_FINALLY(task) \
        case -1:; } \
    (task)->status = UAIO_TERMINATED


#define UAIO_AWAIT(task, entity, coro, ...) \
    do { \
        (task)->current->line = __LINE__; \
        if (entity ## _call_new(task, coro, __VA_ARGS__)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_THROW(task, n) \
    (task)->eno = n; \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_SLEEP(task, us) \
    do { \
        (task)->current->line = __LINE__; \
        uaio_task_sleep(task, us); \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_PASS(task, newstatus) \
    do { \
        (task)->current->line = __LINE__; \
        (task)->status = (newstatus); \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_RETURN(task) \
    (task)->eno = 0; \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_RETHROW(task) \
    (task)->status = UAIO_TERMINATING; \
    return


#define UAIO_HASERROR(task) (task->eno != 0)
#define UAIO_ISERROR(task, e) (UAIO_HASERROR(task) && (task->eno == e))
#define UAIO_CLEARERROR(task) task->eno = 0


#ifdef CONFIG_UAIO_SELECT


/* IO helper macros */
#define UAIO_IN 0x1
#define UAIO_ERR 0x2
#define UAIO_OUT 0x4
#define UAIO_MUSTWAIT(e) \
    (((e) == EAGAIN) || ((e) == EWOULDBLOCK) || ((e) == EINPROGRESS))


int
uaio_file_monitor(struct uaio_task *task, int fd, int events,
        unsigned int timeout_us);


int
uaio_file_forget(int fd);


#define UAIO_FILE_AWAIT(task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if (uaio_file_monitor(task, fd, events, 0)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        else { \
            (task)->status = UAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#define UAIO_FILE_TIMEDOUT(task) ((task)->select_timeout_us < 0)
#define UAIO_FILE_TWAIT(task, fd, events, us) \
    do { \
        (task)->current->line = __LINE__; \
        if (uaio_file_monitor(task, fd, events, us)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        else { \
            (task)->status = UAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


#endif  // CONFIG_UAIO_SELECT


#ifdef CONFIG_UAIO_SEMAPHORE


int
uaio_semaphore_acquire(struct uaio_task *task);


int
uaio_semaphore_release(struct uaio_task *task);


#endif  // CONFIG_UAIO_SEMAPHORE


#define ASYNC void


#endif  // UAIO_H_
