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
#ifndef UAIO_FDMON_H_
#define UAIO_FDMON_H_


#include "uaio/uaio.h"


struct uaio_fdmon;
typedef int (*uaio_filemonitor) (struct uaio_fdmon *iom,
        struct uaio_task *task, int fd, int events);
typedef int (*uaio_fileforget) (struct uaio_fdmon *iom, int fd);
struct uaio_fdmon {
    struct uaio_module;
    uaio_filemonitor monitor;
    uaio_fileforget forget;
};


#define UAIO_FILE_FORGET(fdmon, fd) (fdmon)->forget(fdmon, fd)
#define UAIO_FILE_AWAIT(fdmon, task, fd, events) \
    do { \
        (task)->current->line = __LINE__; \
        if ((fdmon)->monitor(fdmon, task, fd, events)) { \
            (task)->status = UAIO_TERMINATING; \
        } \
        else { \
            (task)->status = UAIO_WAITING; \
        } \
        return; \
        case __LINE__:; \
    } while (0)


/* IO helper macros */
#define UAIO_IN 0x1
#define UAIO_ERR 0x2
#define UAIO_OUT 0x4
#define IO_MUSTWAIT(e) \
    (((e) == EAGAIN) || ((e) == EWOULDBLOCK) || ((e) == EINPROGRESS))


#endif  // UAIO_FDMON_H_
