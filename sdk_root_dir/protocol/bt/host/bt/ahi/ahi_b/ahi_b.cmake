# 向编译目标添加编译选项
set(CFLAG ${ccflags})

# 向编译目标添加编译宏
set(CCDEFINES ${defines})

set(defines ${defines} "BTH_SYS_PART=100")

if(${CHIP} MATCHES "tr5330")
    if(DEFINED BLE_LINUX_CONFIG)
        set(
            bts_ram_src_list
            ${CMAKE_CURRENT_SOURCE_DIR}/ahi_ble/inc
        )
    endif()
else()
set(
    bts_ram_src_list
    ${CMAKE_CURRENT_SOURCE_DIR}/ahi_common/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ahi_ble/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ahi_vendor/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/ahi_bta/inc
)
endif()
set(BTH_PUBLIC_HDR_LIST "${BTH_PUBLIC_HDR_LIST}" "${bts_ram_src_list}" CACHE INTERNAL "bt ram obj list" FORCE)

######################################### service start ######################################

# 添加 host service 子目录
if(${CHIP} MATCHES "tr5310|tr5310p|tr5310pe|tr5310pa|tr5312l|tr5316")
    # add_subdirectory_if_exist(ahi_base)
    # add_subdirectory_if_exist(ahi_mpc)
    add_subdirectory_if_exist(ahi_ble)
    add_subdirectory_if_exist(ahi_vendor)
    add_subdirectory_if_exist(ahi_bta)
elseif(${CHIP} MATCHES "tr5336")
    add_subdirectory_if_exist(ahi_common)
    add_subdirectory_if_exist(ahi_ble)
    add_subdirectory_if_exist(ahi_vendor)
    add_subdirectory_if_exist(ahi_bta)
elseif(${CHIP} MATCHES "trswgxx")
    add_subdirectory_if_exist(ahi_common)
    add_subdirectory_if_exist(ahi_ble)
    add_subdirectory_if_exist(ahi_vendor)
    add_subdirectory_if_exist(ahi_bta)
elseif(${CHIP} MATCHES "tr5330")
    if(DEFINED BLE_LINUX_CONFIG)
        add_subdirectory_if_exist(ahi_ble)
        add_subdirectory_if_exist(ahi_bta)
        add_subdirectory_if_exist(ahi_vendor)
    endif()
else()
    add_subdirectory(ahi_common)
    add_subdirectory(ahi_ble)
    add_subdirectory(ahi_br)
    add_subdirectory(ahi_vendor)
    add_subdirectory(ahi_bta)
endif()
