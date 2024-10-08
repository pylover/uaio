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
#ifndef UAIO_H_  // NOLINT(build/header_guard)
#error "uaio.h must be imported before importing the uaio/generic.h"
#error "And also #undef UAIO_ENTITY, UAIO_ARG1 and UAIO_ARG2 then #define " \
    "UAIO_ENTITY and optionals: UAIO_ARG1/UAIO_ARG2 before importing the " \
    "uaio/generic.h"
#else


typedef void (*UAIO_NAME(coro)) (struct uaio_task *self, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        );  // NOLINT


typedef struct UAIO_NAME(call) {
    struct uaio_basecall;
    UAIO_NAME(coro) coro;
    UAIO_NAME(t) *state;

#ifdef UAIO_ARG1
    UAIO_ARG1 arg1;
    #ifdef UAIO_ARG2
        UAIO_ARG2 arg2;
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
} UAIO_NAME(call);



void
UAIO_NAME(invoker)(struct uaio_task *task);



int
UAIO_NAME(call_new)(struct uaio_task *task, UAIO_NAME(coro) coro,
        UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        );  // NOLINT


int
UAIO_NAME(spawn) (struct uaio *c, UAIO_NAME(coro) coro, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        );  // NOLINT


int
UAIO_NAME(forever) (UAIO_NAME(coro) coro, UAIO_NAME(t) *state
#ifdef UAIO_ARG1
        , UAIO_ARG1 arg1
    #ifdef UAIO_ARG2
            , UAIO_ARG2 arg2
    #endif  // UAIO_ARG2
#endif  // UAIO_ARG1
        , size_t maxtasks);

#endif  // UAIO_H_
