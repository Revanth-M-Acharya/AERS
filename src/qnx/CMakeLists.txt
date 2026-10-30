add_library(aegis_qnx STATIC
    QnxDevctl.cpp
    QnxMessagePassing.cpp
    QnxPulseManager.cpp
    QnxTimerService.cpp
    QnxThreadScheduler.cpp
    QnxInterruptManager.cpp
    QnxResourceManager.cpp
    QnxDiagnostics.cpp
)

target_include_directories(aegis_qnx PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_qnx PUBLIC
    aegis_common
    aegis_logger
    Threads::Threads
)

