add_library(aegis_priority_scheduler STATIC
    priority_scheduler.cpp
)
target_include_directories(aegis_priority_scheduler PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_priority_scheduler PUBLIC aegis_common)
