add_library(aegis_hospital_selection STATIC
    HospitalSelectionEngine.cpp
)

target_include_directories(aegis_hospital_selection PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_hospital_selection PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)
