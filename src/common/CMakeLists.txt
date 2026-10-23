add_library(aegis_common STATIC
    geo_utils.cpp
    time_utils.cpp
    uuid_utils.cpp
    error.cpp
    event.cpp
    timer.cpp
    json_helper.cpp
    validator.cpp
)
target_include_directories(aegis_common PUBLIC ${CMAKE_SOURCE_DIR}/include)
