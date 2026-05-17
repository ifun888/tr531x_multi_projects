#===============================================================================
# @brief    cmake make file
# Copyright (c) Triductor. 2023. All rights reserved.
#===============================================================================
set(AUTO_DEF_FILE_ID FALSE)
if(${CHIP} MATCHES "tr5310|tr5310p|tr5310pe|tr5310pa|tr5312l|tr5316")
    set(BTH_ONETRACK TRUE)
elseif(${CHIP} MATCHES "tr5336")
    set(BTH_ONETRACK TRUE)
else()
    set(BTH_ONETRACK FALSE)
endif()

# 通过定义cmake变量和宏适配没有Kconfig项目
include(KconfigAdapter.cmake)

add_subdirectory_if_exist(bg_common)
add_subdirectory_if_exist(bt)
add_subdirectory_if_exist(gle)
add_subdirectory_if_exist(external)
if(${CHIP} MATCHES "sw39")
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sle_audio/CMakeLists.txt)
        add_subdirectory(sle_audio)
    else() # 有sle_host权限，但没有sle_audio权限时编译库文件
        set(COMPONENT_NAME "bth_saf")
        set(LIBS ${BIN_DIR}/${CHIP}/libs/bluetooth/sleh/${TARGET_COMMAND}/libbth_saf.a)
        set(WHOLE_LINK true)
        build_component()
        install_sdk(${BIN_DIR}/${CHIP}/libs/bluetooth/sleh/${TARGET_COMMAND} "*.a")
    endif()
elseif(${CHIP} MATCHES "sw21")
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sle_audio/CMakeLists.txt)
        add_subdirectory(sle_audio)
    else() # 有sle_host权限，但没有sle_audio权限时编译库文件
        set(COMPONENT_NAME "bth_saf")
        set(LIBS ${BIN_DIR}/${CHIP}/libs/bluetooth/bth/${TARGET_COMMAND}/libbth_saf.a)
        set(WHOLE_LINK true)
        build_component()
        install_sdk(${BIN_DIR}/${CHIP}/libs/bluetooth/bth/${TARGET_COMMAND} "*.a")
    endif()
endif()