#include <errno.h>
#include <stdlib.h>

#include "uaio.h"


struct uaio {
    struct uaio_taskpool taskpool;
    volatile bool terminating;
#ifdef CONFIG_UAIO_MODULES
    struct uaio_module *modules[CONFIG_UAIO_MODULES_MAX];
    size_t modulescount;
#endif  // CONFIG_UAIO_MODULES
};


static struct uaio *_uaio = NULL;


int
uaio_init(size_t maxtasks) {
    return -1;
}


struct uaio*
uaio_create(size_t maxtasks) {
    _uaio = malloc(sizeof(struct uaio));
    if (_uaio == NULL) {
        return -1;
    }

    _uaio->terminating = false;

#ifdef CONFIG_UAIO_MODULES
    _uaio->modulescount = 0;
#endif

    /* Initialize task pool */
    if (uaio_taskpool_init(&_uaio->taskpool, maxtasks)) {
        goto failure;
    }

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

    if (uaio_taskpool_deinit(&_uaio->taskpool)) {
        return -1;
    }

    free(_uaio);
    errno = 0;
    return 0;
}
