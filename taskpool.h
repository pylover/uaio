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
#ifndef UAIO_TASKPOOL_H_
#define UAIO_TASKPOOL_H_


#include "uaio/uaio.h"


struct uaio_taskpool {
    struct uaio_task *tasks;
    struct uaio_task *last;
    size_t size;
    size_t count;
};


int
uaio_taskpool_init(struct uaio_taskpool *p, size_t size);


int
uaio_taskpool_deinit(struct uaio_taskpool *pool);


struct uaio_task *
uaio_taskpool_next(struct uaio_taskpool *pool, struct uaio_task *task,
        enum uaio_taskstatus statuses);


struct uaio_task *
uaio_taskpool_lease(struct uaio_taskpool *pool);


int
uaio_taskpool_release(struct uaio_taskpool *pool, struct uaio_task *task);


#endif  // UAIO_TASKPOOL_H_
