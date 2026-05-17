#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor 2023-2023. All rights reserved.
#===============================================================================
set(SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/server/mouse_sle_service_cfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/mouse_sle_charge_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/mouse_sle_dis_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/mouse_sle_dongle_usb_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/mouse_sle_hid_service.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/server/server_adv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/client/client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/public_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msg/msg.c
)
set(SOURCES "${SOURCES}" ${SOURCES_LIST} PARENT_SCOPE)
