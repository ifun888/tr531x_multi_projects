set(COMPONENT_NAME "samples_custom")

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/ll_tx_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/ll_gpio_mark.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/ll_tx_engine.c
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
set(PUBLIC_HEADER ${PUBLIC_HEADER} PARENT_SCOPE)
