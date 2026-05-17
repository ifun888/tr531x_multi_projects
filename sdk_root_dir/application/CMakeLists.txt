#===============================================================================
# @brief    cmake make file
# Copyright (c) Triductor 2022-2022. All rights reserved.
#===============================================================================
add_subdirectory_if_exist(battery)
add_subdirectory_if_exist(brandy_demo)
add_subdirectory_if_exist(charger)
add_subdirectory_if_exist(dut)
add_subdirectory_if_exist(monitor)
add_subdirectory_if_exist(ota)
add_subdirectory_if_exist(samples)
add_subdirectory_if_exist(ux)
add_subdirectory_if_exist(wstp)
add_subdirectory_if_exist(cfbb)

if("${CHIP}" STREQUAL "socmn1" OR "${CHIP}" STREQUAL "socmn2")
    add_subdirectory_if_exist(melody)
else()
    add_subdirectory_if_exist(${CHIP})
endif()

if("${CHIP}" STREQUAL "socmn1" OR "${CHIP}" STREQUAL "brandy")
    add_subdirectory_if_exist(audio)
endif()

if (${CHIP} STREQUAL "sw21")
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/audio)
        add_subdirectory(audio)
    elseif(EXISTS ${ROOT_DIR}/build/config/target_config/${CHIP}/cmake/audio.cmake)
        include(${ROOT_DIR}/build/config/target_config/${CHIP}/cmake/audio.cmake)
    endif()
endif()

add_subdirectory_if_exist(wearable)
