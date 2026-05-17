#===============================================================================
# @brief    cmake make file
# Copyright (c) Triductor. 2022-2022. All rights reserved.
#===============================================================================
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})
    add_subdirectory_if_exist(${CHIP})
endif()
if(${CHIP_FAMILY} MATCHES "tr531x")
    add_subdirectory_if_exist(${CHIP_FAMILY})
endif()
