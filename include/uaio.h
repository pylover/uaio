#ifndef UAIO_H_
#define UAIO_H_


#include <stddef.h>


enum uaio_taskstatus {
    UAIO_IDLE = 1,
    UAIO_RUNNING = 2,
    UAIO_WAITING = 4,
    UAIO_TERMINATING = 8,
    UAIO_TERMINATED = 16,
};


struct uaio;
struct uaio_task;


int
uaio_init(size_t maxtasks);


int
uaio_destroy();


#endif  // UAIO_H_
