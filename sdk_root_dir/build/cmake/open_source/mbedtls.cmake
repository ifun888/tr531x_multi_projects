#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor. 2022-2022. All rights reserved.
#===============================================================================
if(${CHIP} STREQUAL "brandy")
    if(EXISTS ${ROOT_DIR}/open_source/mbedtls/mbedtls_v3.1.0/harden/build/conncet/mbedtls.cmake)
        include(${ROOT_DIR}/open_source/mbedtls/mbedtls_v3.1.0/harden/build/conncet/mbedtls.cmake)
    endif()
else()
    if(EXISTS ${ROOT_DIR}/drivers/drivers/driver/security_unified/mbedtls_harden_adapt/mbedtls_alt/mbedtls.cmake)
        include(${ROOT_DIR}/drivers/drivers/driver/security_unified/mbedtls_harden_adapt/mbedtls_alt/mbedtls.cmake)
    endif()
endif()

if(mbedtls IN_LIST TARGET_COMPONENT)
    install_sdk(${ROOT_DIR}/open_source/mbedtls "*")
endif()

