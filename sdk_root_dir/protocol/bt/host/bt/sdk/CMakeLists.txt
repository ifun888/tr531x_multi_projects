######################################### sdk start ###########################################
if(${CHIP} MATCHES "tr5330")
    if(DEFINED BLE_LINUX_CONFIG)
        add_subdirectory_if_exist(src)
        add_subdirectory_if_exist(L1)

        set(ble_sdk_linux_hdr
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/L1/include
            ${CMAKE_CURRENT_SOURCE_DIR}/L2/ble
            ${CMAKE_CURRENT_SOURCE_DIR}/L2/common
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_manager
            ${CMAKE_CURRENT_SOURCE_DIR}/src/common
            ${CMAKE_CURRENT_SOURCE_DIR}/src/data_config
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/common/L0
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/ble
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/ble/L0
        )
        set(ble_sdk_linux_src
            ${BTA_RAM_LIST}
        )
        set(BLE_SDK_LINUX_HDR "${BLE_SDK_LINUX_HDR}" "${ble_sdk_linux_hdr}" CACHE INTERNAL "" FORCE)
        set(BLE_SDK_LINUX_SRC "${BLE_SDK_LINUX_SRC}" "${ble_sdk_linux_src}" CACHE INTERNAL "" FORCE)
    endif()
else()
set(BTA_RAM_LIST  "" CACHE INTERNAL "" FORCE)
add_subdirectory_if_exist(src)
add_subdirectory_if_exist(L1)
add_subdirectory_if_exist(vendor)

set(COMPONENT_NAME "bth_sdk")

if("${BTA_RAM_LIST}" STREQUAL "")
    if(DEFINED CONFIG_SLE_BLE_SUPPORT AND SUPPORT_MULTI_LIBS IN_LIST DEFINES)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${PKG_CHIP}-${CONFIG_SLE_BLE_SUPPORT}/lib${COMPONENT_NAME}.a)
    elseif(DEFINED CONFIG_SUPPORT_SLE_BLE_CENTRAL_DEFAULT)
        set(LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_COMMAND}/lib${COMPONENT_NAME}.a)
    else()
        set(SOURCES "__null__")
    endif()
else()
    set(SOURCES
        ${BTA_RAM_LIST}
    )
endif()

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/L1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/L2/ble
    ${CMAKE_CURRENT_SOURCE_DIR}/L2/common
    ${CMAKE_CURRENT_SOURCE_DIR}/L2/br
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../host/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../host/stack_api/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mouse_manager
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mpc_proc
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/common/L0
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/ble
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/ble/L0
)

set(PRIVATE_HEADER
    ${PRIVATE_HEADER}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bg_common/include/ipc
)

if(${CHIP} MATCHES "sw21")
    set(PRIVATE_HEADER
        ${PRIVATE_HEADER}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../bg_common/include/host
        ${CMAKE_CURRENT_SOURCE_DIR}/../../bg_common/portings/log/sw21
    )
endif()

set(PRIVATE_DEFINES
)

set(PUBLIC_DEFINES
)

set(WHOLE_LINK
    true
)

set(LIB_OUT_PATH ${BIN_DIR}/${CHIP}/libs/bluetooth/bth/${TARGET_COMMAND})

build_component()
endif()
######################################### sdk end ###########################################
