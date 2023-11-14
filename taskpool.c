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
#include <stdlib.h>
#include <string.h>

#include "taskpool.h"


int
taskpool_init(struct uaio_taskpool *self, size_t size) {
    self->pool = calloc(size, sizeof(struct uaio_task*));
    if (self->pool == NULL) {
        return -1;
    }
    memset(self->pool, 0, size * sizeof(struct uaio_task*));
    self->count = 0;
    self->size = size;
    return 0;
}


void
taskpool_deinit(struct uaio_taskpool *self) {
    if (self->pool == NULL) {
        return;
    }
    free(self->pool);
}


int
taskpool_append(struct uaio_taskpool *self, struct uaio_task *item) {
    int i;

    if (item == NULL) {
        return -1;
    }

    if (TASKPOOL_ISFULL(self)) {
        return -1;
    }

    for (i = 0; i < self->size; i++) {
        if (self->pool[i] == NULL) {
            goto found;
        }
    }

    /* Not found */
    return -1;

found:
    self->pool[i] = item;
    self->count++;
    return i;
}


int
taskpool_delete(struct uaio_taskpool *self, unsigned int index) {
    if (self->size <= index) {
        return -1;
    }

    self->pool[index] = NULL;
    return 0;
}


struct uaio_task*
taskpool_get(struct uaio_taskpool *self, unsigned int index) {
    if (self->size <= index) {
        return NULL;
    }

    return self->pool[index];
}


void
taskpool_vacuum(struct uaio_taskpool *self) {
    int i;
    int shift = 0;

    for (i = 0; i < self->count; i++) {
        if (self->pool[i] == NULL) {
            shift++;
            continue;
        }

        if (!shift) {
            continue;
        }

        self->pool[i - shift] = self->pool[i];
        self->pool[i - shift]->index = i - shift;
        self->pool[i] = NULL;
    }

    self->count -= shift;
}
