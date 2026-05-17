#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor. 2024-2024. All rights reserved.
#===============================================================================


set(COMPONENT_NAME "gpu_at")

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/at_gpu_cmd_register.c
    ${CMAKE_CURRENT_SOURCE_DIR}/at/at_gpu.c
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/at
)

set(PRIVATE_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/at
    ${ROOT_DIR}/middleware/utils/at/at
    ${ROOT_DIR}/middleware/utils/at/at/include
    ${ROOT_DIR}/middleware/utils/at/at/src
    ${ROOT_DIR}/middleware/services/gpu/drv/include
    ${ROOT_DIR}/middleware/services/gpu/drv/dpu/dfx
)

set(PRIVATE_DEFINES
    LOSCFG_BASE_CORE_USE_MULTI_LIST
)

set(PUBLIC_DEFINES
    AT_COMMAND_GPU
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
    true
)

build_component()

install_sdk(${CMAKE_CURRENT_SOURCE_DIR}/at "*.h")
install_sdk(${CMAKE_CURRENT_SOURCE_DIR}/src "*.h")
