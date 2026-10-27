add_library(aegis_scheduler STATIC
    ConflictDetector.cpp
    PriorityScheduler.cpp
    MissionManager.cpp
    ConflictDetectionEngine.cpp
    IntelligentPriorityScheduler.cpp
    ConflictResolutionManager.cpp
    ManualOverrideManager.cpp
    AmbulanceAwarenessService.cpp
    MultiAmbulanceSimulation.cpp
)

target_include_directories(aegis_scheduler PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_scheduler PUBLIC
    aegis_common
    aegis_ambulance
    aegis_medical
    aegis_logger
    aegis_coordinator
    aegis_communication
)
