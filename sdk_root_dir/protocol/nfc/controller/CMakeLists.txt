message(STATUS "======= current project : ${PROJECT_NAME} - ${project_name} =======")

if (${project_name} MATCHES "6020_device")
    include(ca11.cmake)
elseif (${CHIP} MATCHES "tr5310|tr5310p|tr5310pa|tr5312l|tr5316")
    include(tr5310p.cmake)
elseif (${project_name} MATCHES "5612")
    include(wf56.cmake)
endif()
