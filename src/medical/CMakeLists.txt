add_library(aegis_medical STATIC
    PatientProfile.cpp
    PriorityEngine.cpp
    MedicalDatabase.cpp
)

target_include_directories(aegis_medical PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_medical PUBLIC
    aegis_common
)
