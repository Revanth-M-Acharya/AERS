add_library(aegis_recovery STATIC
    FaultRecoveryManager.cpp
)

target_include_directories(aegis_recovery PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_recovery PUBLIC
    aegis_common
    aegis_logger
)

