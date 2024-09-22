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
#ifndef UAIO_SLEEP_H_
#define UAIO_SLEEP_H_


#include <sys/timerfd.h>

#include "uaio/uaio.h"
#include "uaio/fdmon.h"


typedef int uaio_sleep_t;


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY uaio_sleep
#define UAIO_ARG1 struct uaio_fdmon *
#define UAIO_ARG2 time_t
#include "uaio/generic.h"


int
uaio_sleep_create(uaio_sleep_t *sleep);


int
uaio_sleep_destroy(uaio_sleep_t *sleep);


ASYNC
uaio_sleepA(struct uaio_task *self, uaio_sleep_t *state,
        struct uaio_fdmon *iom, time_t miliseconds);


#define UAIO_SLEEP(self, state, iom, miliseconds) \
    UAIO_AWAIT(self, uaio_sleep, uaio_sleepA, state, \
            (struct uaio_fdmon*)iom, miliseconds)


#endif  // UAIO_SLEEP_H_
