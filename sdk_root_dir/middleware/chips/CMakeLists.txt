#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor. 2022-2022. All rights reserved.
#===============================================================================
add_subdirectory_if_exist(${CHIP})

if(${CHIP_FAMILY} MATCHES "tr531x")
    add_subdirectory_if_exist(${CHIP_FAMILY})
endif()