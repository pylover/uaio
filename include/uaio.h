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


enum uaio_taskstatus {
    UAIO_IDLE = 1,
    UAIO_RUNNING = 2,
    UAIO_WAITING = 4,
    UAIO_TERMINATING = 8,
    UAIO_TERMINATED = 16,
};


struct uaio;
struct uaio_task;


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


#ifdef CONFIG_UAIO_SELECT


/* IO helper macros */
#define UAIO_IN 0x1
#define UAIO_ERR 0x2
#define UAIO_OUT 0x4
#define UAIO_MUSTWAIT(e) \
    (((e) == EAGAIN) || ((e) == EWOULDBLOCK) || ((e) == EINPROGRESS))


int
uaio_select_monitor(struct uaio_task *task, int fd, int events,
        unsigned int timeout_us);


int
uaio_select_forget(int fd);


#endif  // CONFIG_UAIO_SELECT


#ifdef CONFIG_UAIO_UART

#endif  // CONFIG_UAIO_UART


#endif  // UAIO_H_
