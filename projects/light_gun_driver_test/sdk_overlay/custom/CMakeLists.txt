set(COMPONENT_NAME "samples_custom")

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_sle_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/solenoid_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rumble_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ir_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_sim_test.c
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
