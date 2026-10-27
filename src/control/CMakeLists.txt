add_library(aegis_control STATIC
    ManualOverride.cpp
    EmergencyOperatorConsole.cpp
)

target_include_directories(aegis_control PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_control PUBLIC
    aegis_common
    aegis_ambulance
    aegis_scheduler
    aegis_medical
)
