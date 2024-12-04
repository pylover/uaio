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
#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_


#include "uaio.h"


typedef struct uaio_semaphore {
    volatile int value;
    struct uaio_task *task;
} uaio_semaphore_t;


int
uaio_semaphore_begin(struct uaio_task *task, struct uaio_semaphore *s);


int
uaio_semaphore_end(struct uaio_task *task);


#endif  // SEMAPHORE_H_
