idf_component_register(
  SRCS 
    uaio.c
    taskpool.c
  INCLUDE_DIRS "include"
  REQUIRES elog esp_driver_uart esp_hw_support esp_timer
)


# include_directories(
#   $ENV{HERE}/build/esp-idf/uaio
#   $ENV{HERE}/components/esp_hw_support/include
# )


target_compile_options(${COMPONENT_LIB} PRIVATE -fms-extensions)

