set(sources
  "uaio.c"
  "taskpool.c"
)


if (CONFIG_UAIO_SEMAPHORE) 
  list(APPEND sources
    "semaphore.c"
  )
endif()


if (CONFIG_UAIO_SELECT) 
  list(APPEND sources
    "select.c"
  )
endif()


idf_component_register(
  SRCS "${sources}"
  INCLUDE_DIRS "include"
  REQUIRES elog esp_timer freertos
)


target_compile_options(${COMPONENT_LIB} PRIVATE -fms-extensions)
