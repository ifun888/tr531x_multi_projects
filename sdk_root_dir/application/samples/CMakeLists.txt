#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor. 2023-2023. All rights reserved.
#===============================================================================
set(COMPONENT_NAME "samples")

set(SOURCES
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/public/server
    ${CMAKE_CURRENT_SOURCE_DIR}/public/client
    ${CMAKE_CURRENT_SOURCE_DIR}/public/msg
    ${CMAKE_CURRENT_SOURCE_DIR}/mp_test
)

if(DEFINED CONFIG_ENABLE_BT_SAMPLE)
    add_subdirectory_if_exist(bt)
endif()
if(DEFINED CONFIG_ENABLE_PERIPHERAL_SAMPLE)
    add_subdirectory_if_exist(peripheral)
endif()
if(DEFINED CONFIG_ENABLE_WIFI_SAMPLE)
    add_subdirectory_if_exist(wifi)
endif()
if(DEFINED CONFIG_ENABLE_PRODUCTS_SAMPLE)
    add_subdirectory_if_exist(products)
endif()
if(DEFINED CONFIG_ENABLE_RADAR_SAMPLE)
    add_subdirectory_if_exist(radar)
endif()
if(DEFINED CONFIG_ENABLE_NFC_SAMPLE)
    add_subdirectory_if_exist(nfc)
endif()
if(DEFINED CONFIG_ENABLE_PRODUCTS_SAMPLE)
    add_subdirectory_if_exist(mp_test)
endif()
if(DEFINED CONFIG_ENABLE_PRODUCTS_SAMPLE AND DEFINED CONFIG_SAMPLE_SUPPORT_MOUSE)
    add_subdirectory_if_exist(public)
endif()

add_subdirectory_if_exist(custom)

set(PRIVATE_HEADER
)

set(PRIVATE_DEFINES
)

set(PUBLIC_DEFINES
)

# use this when you want to add ccflags like -include xxx
set(COMPONENT_PUBLIC_CCFLAGS
)

set(COMPONENT_CCFLAGS
)

set(WHOLE_LINK
    true
)

set(MAIN_COMPONENT
    false
)

if(${CHIP_FAMILY} MATCHES "tr531x")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/peripheral" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/products" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/nfc" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/public" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/mp_test" "*")
else()
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/peripheral" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/products" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/sta_sample" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/softap_sample" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/tcp_sample" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/ohos_connect" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/hilink_indie_upgrade" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/wifi/ble_wifi_cfg_sample" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/radar" "*")
    install_sdk("${CMAKE_CURRENT_SOURCE_DIR}/nfc" "*")
endif()

build_component()
