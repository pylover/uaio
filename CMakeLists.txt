add_library(uaio INTERFACE) 
target_sources(uaio PUBLIC 
    uaio.c uaio.h
    sleep.c
    taskpool.c taskpool.h
)
