#===============================================================================
# @brief    cmake file
# Copyright (c) Triductor.  2024-2024. All rights reserved.
#===============================================================================
set(COMPONENT_NAME "flashboot_init_porting")

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/flashboot_init.c
)

set(PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(PRIVATE_HEADER
)

if(NOT DEFINED CONFIG_BOOT_SUPPORT_SECURE_VERIFY_V2)
    list(APPEND PUBLIC_HEADER "${ROOT_DIR}/bootloader/commonboot/include/secure_verify_boot_v1")
else()
    list(APPEND PUBLIC_HEADER "${ROOT_DIR}/bootloader/commonboot/include/secure_verify_boot_v2")
endif()

if(DEFINED CONFIG_DRIVERS_USB_DFU_GADGET OR DEFINED CONFIG_DRIVERS_USB_HID_GADGET)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/usb_download/usb_download.c)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/usb_download/usb_boot_info.c)
    list(APPEND PRIVATE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/usb_download)
endif()

set(PRIVATE_DEFINES
)

if(DEFINED CONFIG_DRIVERS_USB_DFU_GADGET OR DEFINED CONFIG_DRIVERS_USB_HID_GADGET)
set(PUBLIC_DEFINES
    FLASHBOOT_LOAD_BY_USB
)
endif()

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