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
#ifndef UAIO_SELECT_H_
#define UAIO_SELECT_H_


#include <sys/select.h>

#include "uaio/fdmon.h"
#include "uaio/uaio.h"


struct uaio_select;


struct uaio_select *
uaio_select_create(struct uaio* c, size_t maxfileno);


int
uaio_select_destroy(struct uaio* c, struct uaio_select *s);


int
uaio_select_monitor(struct uaio_select *s, struct uaio_task *task, int fd,
        int events);


int
uaio_select_forget(struct uaio_select *s, int fd);


#endif  // UAIO_SELECT_H_
