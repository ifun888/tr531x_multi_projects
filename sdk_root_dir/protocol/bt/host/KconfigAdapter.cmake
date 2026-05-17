# 此文件为不支持Kconfig项目定义Cmake变量和宏

# tr5310p项目A核支持Kconfig,B核不支持Kconfig
if(${CHIP} MATCHES "tr5312")
    add_definitions(-D CONFIG_FEATURE_GLE_HADM)
    set(CONFIG_FEATURE_GLE_HADM "")
endif()