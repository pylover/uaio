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
#include <unistd.h>

#include "stm32l0xx.h"

#include "uaio.h"
#include "sleep.h"


static struct uaio_task *timer2 = NULL;


void
TIM2_IRQHandler() {
    if (!(TIM2->SR & TIM_SR_UIF)) {
        return;
    }

    if (timer2 == NULL) {
        return;
    }

    REG_CLEAR(TIM2->SR, TIM_SR_UIF);
    REG_CLEAR(TIM2->CR1, TIM_CR1_CEN);
    timer2->status = UAIO_RUNNING;
    timer2 = NULL;
}


ASYNC
uaio_sleepA(struct uaio_task *self, int miliseconds) {
    CORO_START;

    if (timer2 != NULL) {
        ERROR("Timer busy");
        CORO_RETURN;
    }

    TIM2->CNT = miliseconds;
    TIM2->ARR = miliseconds;
    TIM2->EGR |= TIM_EGR_UG;

    timer2 = self;
    UAIO_IWAIT(TIM2->CR1 |= TIM_CR1_CEN);

    CORO_FINALLY;
}
