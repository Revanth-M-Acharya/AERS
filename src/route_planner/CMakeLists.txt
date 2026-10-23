add_library(aegis_route_planner STATIC
    route_planner.cpp
)
target_include_directories(aegis_route_planner PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_route_planner PUBLIC aegis_common aegis_config)
