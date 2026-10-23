add_library(aegis_common STATIC
    geo_utils.cpp
)
target_include_directories(aegis_common PUBLIC ${CMAKE_SOURCE_DIR}/include)
