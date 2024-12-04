#ifndef SELECT_H_
#define SELECT_H_


#include <stddef.h>

#include "uaio.h"


#define FILEEVENT_RESET(fe) \
            (fe)->task = NULL; \
            (fe)->fd = -1; \
            (fe)->events = 0


struct uaio_fileevent {
    int fd;
    int events;
    struct uaio_task *task;
};


struct uaio_select {
    unsigned int maxfileno;
    size_t waitingfiles;
    struct uaio_fileevent *events;
    size_t eventscount;
};


int
uaio_select_tick(struct uaio_select *s, unsigned int timeout_us);


#endif   // SELECT_H_
