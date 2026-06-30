set(COMPONENT_NAME "samples_custom")

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/of_entry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/of_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/of_diag.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sm/of_sm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sm/sm_selftest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sm/sm_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sm/sm_link.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sm/sm_usb_sle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/transport_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/transport_sle_sdk_hook.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/transport_rx_hooks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/of_sle_uart_client_local.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/of_wireless_pkt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/of_link_io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_usb_cdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_sle_link.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_ir_cam.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_input_keys.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_input_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_feedback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_solenoid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_rumble.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_usb_hid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drv_temp_sensor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_transport_router.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_profile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_calibration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_binding.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_position.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services/svc_usb_hid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocols/proto_docked.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocols/proto_mh.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/of_time.c
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES ${SOURCES} PARENT_SCOPE)
set(PUBLIC_HEADER ${PUBLIC_HEADER} PARENT_SCOPE)
