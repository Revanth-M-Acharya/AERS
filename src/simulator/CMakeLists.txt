add_library(aegis_simulator STATIC
    CityClock.cpp
    RoadGraph.cpp
    VirtualVehicle.cpp
    VirtualAmbulance.cpp
    VirtualIntersection.cpp
    VirtualHospital.cpp
    EmergencyGenerator.cpp
    CityModel.cpp
    DashboardAdapter.cpp
    SimulationEngine.cpp
)

target_include_directories(aegis_simulator PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
if(Qt6_FOUND)
    target_link_libraries(aegis_simulator PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
    )
endif()

target_link_libraries(aegis_simulator PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

