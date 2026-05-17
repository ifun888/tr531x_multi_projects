#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor 2022-2022. All rights reserved.
#===============================================================================
set(COMPONENT_NAME "bth_gle")

if(${CHIP} MATCHES "sw39")
    set(BTH_ROM_VERSION true)
else()
    set(BTH_ROM_VERSION false)
endif()

set(BTH_RAM_LIST  "" CACHE INTERNAL "" FORCE)
set(BTH_PUBLIC_HEADER_LIST "" CACHE INTERNAL "" FORCE)
set(BTH_PRIVATE_HEADER_LIST "" CACHE INTERNAL "" FORCE)
add_subdirectory_if_exist(host)
add_subdirectory_if_exist(ahi)
add_subdirectory_if_exist(dft)
add_subdirectory_if_exist(samples)
add_subdirectory_if_exist(sdk)
add_subdirectory_if_exist(service)
add_subdirectory_if_exist(utils)
if("${BTH_RAM_LIST}" STREQUAL "")
    if(DEFINED CONFIG_SLE_BLE_SUPPORT AND SUPPORT_MULTI_LIBS IN_LIST DEFINES)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${PKG_CHIP}-${CONFIG_SLE_BLE_SUPPORT}/lib${COMPONENT_NAME}.a)
    elseif(DEFINED CONFIG_SUPPORT_SLE_BLE_CENTRAL_DEFAULT)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_COMMAND}/lib${COMPONENT_NAME}.a)
    else()
        set(BTH_RAM_LIST "__null__")
    endif()
endif()
set(SOURCES
    ${BTH_RAM_LIST}
)

MESSAGE("BTH_PUBLIC_HEADER_LIST=" ${BTH_PUBLIC_HEADER_LIST})

set(PUBLIC_HEADER
    ${BTH_PUBLIC_HEADER_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/L0
    ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/sdk_common
)

if(${CHIP} MATCHES "sw39")
    set(PRIVATE_HEADER
        ${BTH_PRIVATE_HEADER_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/portings/nv/common/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/portings/tladapter/sle_only
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/config/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/portings/log/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/service
    )
else()
    set(PRIVATE_HEADER
        ${BTH_PRIVATE_HEADER_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/ipc
    )
endif()

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

if(${CHIP} MATCHES "sw39")
    set(LIB_OUT_PATH ${BIN_DIR}/${CHIP}/libs/bluetooth/sleh/${TARGET_COMMAND})
else()
    set(LIB_OUT_PATH ${BIN_DIR}/${CHIP}/libs/bluetooth/bth/${TARGET_COMMAND})
endif()

build_component()

MESSAGE("BTH_ROM_VERSION=" ${BTH_ROM_VERSION})
if(BTH_ROM_VERSION)
    set(COMPONENT_NAME "bth_gle_rom")
    set(SOURCES
        ${BTH_ROM_LIST}
    )
    MESSAGE("bth_gle_rom BTH_ROM_LIST:" ${BTH_ROM_LIST})
    set(PRIVATE_HEADER
        ${BTH_PRIVATE_HEADER_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/portings/nv/common/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/config/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/portings/log/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/../bg_common/include/service
    )
    set(PUBLIC_HEADER
        ${BTH_PUBLIC_HEADER_LIST}
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
    build_component()
endif()
