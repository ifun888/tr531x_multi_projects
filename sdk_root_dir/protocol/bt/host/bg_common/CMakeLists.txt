#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor. 2023. All rights reserved.
#===============================================================================
# Temporarily Released
if(${CHIP} MATCHES "bisheng")
include(gle_host_offload_noneos.cmake)
else()
if(${CHIP} MATCHES "sw39|sw21")
    set(MODULE_NAME "bgh")
else()
    set(MODULE_NAME "bth")
endif()
set(AUTO_DEF_FILE_ID FALSE)
set(COMPONENT_NAME "bg_common")

set(BTH_RAM_LIST  "" CACHE INTERNAL "" FORCE)
set(BTH_ROM_LIST  "" CACHE INTERNAL "" FORCE)

if(DEFINED ROM_COMPONENT OR DEFINED ROM_SYM_PATH)
    set(BT_ROM_VERSION true)
else()
    set(BT_ROM_VERSION false)
endif()

if(${CHIP_FAMILY} MATCHES "tr531x")
    add_subdirectory_if_exist(chips)
    add_subdirectory_if_exist(src/utils)
    if (SUPPORT_CHIP_N1200 IN_LIST DEFINES)
        set(PUBLIC_HEADER
            ${BTH_PUBLIC_HDR_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/oml
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log
        )
    else()
        set(PUBLIC_HEADER
            ${BTH_PUBLIC_HDR_LIST}
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/tr5310p
        )
    endif()

elseif((${CHIP} MATCHES "tr5312") AND ("bt_host_rom" IN_LIST TARGET_COMPONENT))
    set(MODULE_NAME "bt")
    add_subdirectory_if_exist(chips)
    set(PUBLIC_HEADER
        ${BTH_PUBLIC_HDR_LIST}
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/tr5312
    )
elseif(${CHIP} MATCHES "tr5336")
    add_subdirectory_if_exist(chips)
    add_subdirectory_if_exist(src)
    set(PUBLIC_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/nv
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_b
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/include/config/tr5336
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/oml
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/../host/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dfx
        ${CMAKE_CURRENT_SOURCE_DIR}/include/power_on
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nv
    )
elseif(${CHIP} MATCHES "trswgxx")
    add_subdirectory_if_exist(src)
    set(PUBLIC_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/nv
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/tladapter/hcc
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/tladapter/os
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_b
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/include/config/trswgxx
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/uapi
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/include/power_on
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dfx
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nv
    )
elseif(${CHIP} MATCHES "sw21")
    add_subdirectory_if_exist(src)
    set(PRIVATE_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw21
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host
    )
    set(PUBLIC_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/nv
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/tladapter/sle_only
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_b
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/include/power_on
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dfx
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vendor
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nv
    )
    if(DEFINED "SW21_WSTP" OR "SW21_WSTP" IN_LIST DEFINES)
        set(PUBLIC_HEADER
            ${PUBLIC_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/tladapter/os
            ${CMAKE_CURRENT_SOURCE_DIR}/include/config/sw21/wstp
        )
    else()
        set(PUBLIC_HEADER
            ${PUBLIC_HEADER}
            ${CMAKE_CURRENT_SOURCE_DIR}/include/config/sw21
        )
    endif()
elseif(${CHIP} MATCHES "sw39")
    add_subdirectory_if_exist(src)
    set(PRIVATE_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/nv
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/tladapter/sle_only
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_b
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/include/config/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw39
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dfx
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vendor
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nv
    )
elseif(${CHIP} MATCHES "tr5312|brandy")
    add_subdirectory_if_exist(src)
    set(PUBLIC_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/nv/tr5312/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_a
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_b
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ahi_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hci
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/include/host/stack
        ${CMAKE_CURRENT_SOURCE_DIR}/include/config/default
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/oml
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk_common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/service
        ${CMAKE_CURRENT_SOURCE_DIR}/include/at
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dfx
    )
endif()
add_subdirectory_if_exist(portings)
set(PUBLIC_HEADER
        ${PUBLIC_HEADER}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/config
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/nv
)
if("${BTH_RAM_LIST}" STREQUAL "")
    if(DEFINED CONFIG_SLE_BLE_SUPPORT AND SUPPORT_MULTI_LIBS IN_LIST DEFINES)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${PKG_CHIP}-${CONFIG_SLE_BLE_SUPPORT}/lib${COMPONENT_NAME}.a)
    elseif(DEFINED CONFIG_SUPPORT_SLE_BLE_CENTRAL_DEFAULT)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_COMMAND}/lib${COMPONENT_NAME}.a)
    else()
        set(BTH_RAM_LIST "__null__")
    endif()
elseif(${CHIP_FAMILY} MATCHES "tr531x")
    if (SUPPORT_CHIP_N1200 IN_LIST DEFINES) # ROM-EC后
        set(LOG_DEF
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/log_def_bth.h
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/log_def_sdk_bth.h
        )
    else() # ROM-EC前
        set(LOG_DEF
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/tr5310p/log_def_bth.h
            ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/tr5310p/log_def_sdk_bth.h
        )
    endif()
elseif(${CHIP} MATCHES "sw39")
    set(LOG_DEF
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw39/log_def_bgh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw39/log_def_sdk_bgh.h
    )
elseif(${CHIP} MATCHES "sw21")
    set(LOG_DEF
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw21/log_def_bgh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/sw21/log_def_sdk_bgh.h
    )
else()
    set(LOG_DEF
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/log_def_bth.h
        ${CMAKE_CURRENT_SOURCE_DIR}/portings/log/log_def_sdk_bth.h
    )
endif()

set(SOURCES
    ${BTH_RAM_LIST}
)

if((NOT "ipc_old" IN_LIST TARGET_COMPONENT) AND (NOT "tr5312_bgle" IN_LIST TARGET_COMPONENT))
set(PRIVATE_HEADER
    ${PRIVATE_HEADER}
    # Temporarily Released
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ipc
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


set(COMPONENT_NAME "bg_common_rom")

if("${BTH_ROM_LIST}" STREQUAL "")
    set(BTH_ROM_LIST "__null__")
endif()

MESSAGE("BG COMMON BTH_ROM_LIST:" ${BTH_ROM_LIST})
set(SOURCES
    ${BTH_ROM_LIST}
)

set(PUBLIC_HEADER
    ${BTH_PUBLIC_HDR_LIST}
)

set(PRIVATE_HEADER
    ${BTH_PRIVATE_HDR_LIST}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ipc
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
MESSAGE("BTH_ROM_LIST:" ${BTH_ROM_LIST})
set(LIB_OUT_PATH ${BIN_DIR}/${CHIP}/libs/bluetooth/bth/${TARGET_COMMAND})

build_component()

endif()
