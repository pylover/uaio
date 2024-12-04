#include <errno.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sleep.h>

#include "uaio.h"
#include "taskpool.h"
#include "select.h"
#include "semaphore.h"


struct uaio {
    struct uaio_taskpool taskpool;
#ifdef CONFIG_UAIO_SELECT
    struct uaio_select select;
#endif
};


static struct uaio *_uaio = NULL;


static void
_tasksleep_callback(struct uaio_task *task) {
    // esp_timer_delete(task->sleep);
    ESP_ERROR_CHECK(esp_timer_delete(task->sleep));
    task->sleep = NULL;

    if (task && (task->status == UAIO_WAITING)) {
        task->status = UAIO_RUNNING;
    }
}


void
uaio_task_sleep(struct uaio_task *task, unsigned long us) {
    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = (void (*)(void *)) &_tasksleep_callback,
            .arg = (void*) task
    };
    task->status = UAIO_WAITING;
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &task->sleep));
    ESP_ERROR_CHECK(esp_timer_start_once(task->sleep, us));
}


#ifdef CONFIG_UAIO_SELECT


int
uaio_file_monitor(struct uaio_task *task, int fd, int events,
        unsigned int timeout_us) {
    struct uaio_select *s = &_uaio->select;
    struct uaio_fileevent *fe;
    if ((fd < 0) || (fd > s->maxfileno) || (s->eventscount == s->maxfileno)) {
        return -1;
    }

    if (timeout_us > 0) {
        clock_gettime(CLOCK_MONOTONIC, &task->select_timestamp);
        task->select_timeout_us = timeout_us;
    }
    else {
        task->select_timestamp.tv_sec = 0;
        task->select_timestamp.tv_nsec = 0;
        task->select_timeout_us = 0;
    }

    fe = &s->events[s->eventscount++];
    s->waitingfiles++;
    fe->events = events;
    fe->task = task;
    fe->fd = fd;
    return 0;
}


int
uaio_select_forget(int fd) {
    int i;
    struct uaio_fileevent *fe;

    for (i = 0; i < _uaio->select.eventscount; i++) {
        fe = &_uaio->select.events[i];
        if (fe->fd == fd) {
            FILEEVENT_RESET(fe);
            _uaio->select.waitingfiles--;
            return 0;
        }
    }

    return -1;
}


#endif


struct uaio_task *
uaio_task_next(struct uaio *c, struct uaio_task *task,
        enum uaio_taskstatus statuses) {
    return uaio_taskpool_next(&c->taskpool, task, statuses);
}


void
uaio_task_killall() {
    struct uaio_task *task = NULL;

    while ((task = uaio_taskpool_next(&_uaio->taskpool, task,
                    UAIO_RUNNING | UAIO_WAITING))) {
        task->status = UAIO_TERMINATING;
    }
}


static inline bool
_step(struct uaio_task *task) {
    struct uaio_basecall *call = task->current;

start:
    /* Pre execution */
    if (task->status == UAIO_TERMINATING) {
        /* Tell coroutine to jump to the CORO_FINALLY label */
        call->line = -1;
    }

    call->invoke(task);

    /* Post execution */
    if (task->status == UAIO_TERMINATING) {
        goto start;
    }

    if (task->status == UAIO_TERMINATED) {
        task->current = call->parent;
        free(call);
        if (task->current != NULL) {
            task->status = UAIO_RUNNING;
        }
    }

    return task->current == NULL;
}


struct uaio_task *
uaio_task_new() {
    struct uaio_task *task;

    /* Register task */
    task = uaio_taskpool_lease(&_uaio->taskpool);
    if (task == NULL) {
        return NULL;
    }

    return task;
}


int
uaio_task_dispose(struct uaio_task *task) {
    return uaio_taskpool_release(&_uaio->taskpool, task);
}


int
uaio_init(size_t maxtasks) {
    _uaio = malloc(sizeof(struct uaio));
    if (_uaio == NULL) {
        return -1;
    }

    /* Initialize task pool */
    if (uaio_taskpool_init(&_uaio->taskpool, maxtasks)) {
        goto failure;
    }


#ifdef CONFIG_UAIO_SELECT
    /* Select module */
    memset(&_uaio->select, 0, sizeof(struct uaio_select));
    /* select(2) requires the highest number of fileno instead of event count.
     * So, it must increased 3 times for (stdin, stdout and stderr) */
    _uaio->select.maxfileno = CONFIG_UAIO_SELECT_MAXFILES + 3;
    _uaio->select.events = calloc(_uaio->select.maxfileno,
            sizeof(struct uaio_fileevent));
    _uaio->select.eventscount = 0;
    if (_uaio->select.events == NULL) {
        goto failure;
    }

#endif  // CONFIG_UAIO_SELECT

    return 0;

failure:
    uaio_destroy(_uaio);
    return -1;
}


int
uaio_destroy() {
    if (_uaio == NULL) {
        return -1;
    }

    if (_uaio->select.events) {
        free(_uaio->select.events);
    }

    if (uaio_taskpool_deinit(&_uaio->taskpool)) {
        return -1;
    }

    free(_uaio);
    errno = 0;
    return 0;
}


int
uaio_loop() {
    struct uaio_task *task = NULL;
    struct uaio_taskpool *taskpool = &_uaio->taskpool;
    unsigned int modtimeout = CONFIG_UAIO_TICKTIMEOUT_SHORT_US;
    TickType_t xdelay;

loop:

    while (taskpool->count) {
#ifdef CONFIG_UAIO_SELECT
        if (uaio_select_tick(&_uaio->select, modtimeout)) {
            goto interrupt;
        }
#endif
        task = uaio_taskpool_next(taskpool, task,
                UAIO_RUNNING | UAIO_TERMINATING);
        if (task == NULL) {
            modtimeout = CONFIG_UAIO_TICKTIMEOUT_LONG_US;
            xdelay = modtimeout / 1000 / portTICK_PERIOD_MS;
            vTaskDelay(xdelay);
            continue;
        }

        do {
            /* feed the watchdog */
            vTaskDelay(1 / portTICK_PERIOD_MS);
            if (_step(task)) {
#ifdef CONFIG_UAIO_SEMAPHORE
                if (task->semaphore) {
                    uaio_semaphore_release(task);
                }
#endif
                uaio_taskpool_release(taskpool, task);
            }
        } while ((task = uaio_taskpool_next(taskpool, task,
                    UAIO_RUNNING | UAIO_TERMINATING)));
        modtimeout = CONFIG_UAIO_TICKTIMEOUT_SHORT_US;
    }

    return 0;

interrupt:
    uaio_task_killall();
    goto loop;
}
