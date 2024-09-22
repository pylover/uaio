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
#include <errno.h>

#include "uaio/uaio.h"
#include "uaio/sleep.h"


#undef UAIO_ARG1
#undef UAIO_ARG2
#undef UAIO_ENTITY
#define UAIO_ENTITY uaio_sleep
#define UAIO_ARG1 struct uaio_fdmon *
#define UAIO_ARG2 time_t
#include "uaio/generic.c"


int
uaio_sleep_create(uaio_sleep_t *sleep) {
    int fd;
    if (sleep == NULL) {
        return -1;
    }
    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (fd == -1) {
        return -1;
    }

    *sleep = fd;
    return 0;
}


int
uaio_sleep_destroy(uaio_sleep_t *sleep) {
    if (sleep == NULL) {
        return -1;
    }

    return close(*sleep);
}


static int
_settimeout(int fd, time_t miliseconds) {
    if (fd == -1) {
        errno = EINVAL;
        return -1;
    }

    struct timespec sec = {miliseconds / 1000, (miliseconds % 1000) * 1000};
    struct timespec zero = {0, 0};
    struct itimerspec spec = {zero, sec};
    if (timerfd_settime(fd, 0, &spec, NULL) == -1) {
        return -1;
    }

    return 0;
}


ASYNC
uaio_sleepA(struct uaio_task *self, uaio_sleep_t *state,
        struct uaio_fdmon *iom, time_t miliseconds) {
    int eno;
    int fd = *state;
    UAIO_BEGIN(self);

    if (_settimeout(fd, miliseconds)) {
        eno = errno;
        close(fd);
        *state = -1;
        UAIO_THROW(self, eno);
    }

    UAIO_FILE_AWAIT(iom, self, fd, UAIO_IN);
    UAIO_FILE_FORGET(iom, fd);
    UAIO_FINALLY(self);
}
