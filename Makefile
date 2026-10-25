# ==============================================================================
# SMART AEGIS: Universal Build Automation Makefile
# ==============================================================================

CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
INCLUDES = -Iinclude

BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Release)
    CXXFLAGS += -O3 -DNDEBUG
else
    CXXFLAGS += -g3 -O0 -DAEGIS_DEBUG
endif

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj

# Library Sources
COMMON_SRCS = src/common/geo_utils.cpp src/common/time_utils.cpp src/common/uuid_utils.cpp \
              src/common/error.cpp src/common/event.cpp src/common/timer.cpp \
              src/common/json_helper.cpp src/common/validator.cpp

CONFIG_SRCS = src/config/config_manager.cpp
LOGGER_SRCS = src/logger/logger.cpp
SAFETY_SRCS = src/safety/watchdog.cpp src/safety/safety_monitor.cpp
NETWORK_SRCS = src/network/retry_policy.cpp src/network/serializer.cpp \
               src/network/message_router.cpp src/network/authentication_manager.cpp \
               src/network/connection_manager.cpp src/network/socket_client.cpp \
               src/network/socket_server.cpp src/network/heartbeat_manager.cpp \
               src/network/mqtt_client.cpp src/network/network_manager.cpp
SCHED_SRCS = src/priority_scheduler/priority_scheduler.cpp
ROUTE_SRCS = src/route_planner/route_planner.cpp
COORD_SRCS = src/coordinator/emergency_call_handler.cpp \
             src/coordinator/incident_manager.cpp \
             src/coordinator/ambulance_registry.cpp \
             src/coordinator/fleet_manager.cpp \
             src/coordinator/dispatch_engine.cpp \
             src/coordinator/dynamic_green_corridor.cpp \
             src/coordinator/coordinator.cpp
AMB_SRCS = src/ambulance/gps_tracker.cpp src/ambulance/vitals_monitor.cpp src/ambulance/v2v_module.cpp src/ambulance/ambulance_node.cpp src/ambulance/ambulance_mission_simulator.cpp
TRAFFIC_SRCS = src/traffic_controller/gpio_interface.cpp \
               src/traffic_controller/signal_state_machine.cpp \
               src/traffic_controller/preemption_manager.cpp \
               src/traffic_controller/recovery_manager.cpp \
               src/traffic_controller/traffic_controller.cpp \
               src/traffic_controller/traffic_signal_node.cpp
HOSP_SRCS = src/hospital_dashboard/hospital_dashboard_backend.cpp
TRFDASH_SRCS = src/traffic_dashboard/traffic_dashboard_backend.cpp

ALL_SRCS = $(COMMON_SRCS) $(CONFIG_SRCS) $(LOGGER_SRCS) $(SAFETY_SRCS) $(NETWORK_SRCS) \
           $(SCHED_SRCS) $(ROUTE_SRCS) $(COORD_SRCS) $(AMB_SRCS) $(TRAFFIC_SRCS) \
           $(HOSP_SRCS) $(TRFDASH_SRCS)

ALL_OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SRCS))

STATIC_LIB = $(LIB_DIR)/libaegis.a

# App Executables & Tests
APPS = $(BIN_DIR)/aegis_coordinator $(BIN_DIR)/aegis_ambulance $(BIN_DIR)/aegis_traffic_controller
SMOKE_TEST_BIN = $(BIN_DIR)/test_build_smoke
FOUNDATION_TEST_BIN = $(BIN_DIR)/test_common_foundation
COMM_TEST_BIN = $(BIN_DIR)/test_communication_infrastructure
COORD_TEST_BIN = $(BIN_DIR)/test_coordinator_core
AMB_TEST_BIN = $(BIN_DIR)/test_ambulance_node
TRAFFIC_TEST_BIN = $(BIN_DIR)/test_traffic_signal_node

.PHONY: all clean test directories info

all: directories $(STATIC_LIB) $(APPS) $(SMOKE_TEST_BIN) $(FOUNDATION_TEST_BIN) $(COMM_TEST_BIN) $(COORD_TEST_BIN) $(AMB_TEST_BIN) $(TRAFFIC_TEST_BIN)
	@echo "=========================================================="
	@echo " SMART AEGIS Build Complete [$(BUILD_TYPE)]"
	@echo " Binaries located in: $(BIN_DIR)"
	@echo "=========================================================="

directories:
	@mkdir -p $(BIN_DIR) $(LIB_DIR)
	@mkdir -p $(OBJ_DIR)/common $(OBJ_DIR)/config $(OBJ_DIR)/logger $(OBJ_DIR)/safety
	@mkdir -p $(OBJ_DIR)/network $(OBJ_DIR)/priority_scheduler $(OBJ_DIR)/route_planner
	@mkdir -p $(OBJ_DIR)/coordinator $(OBJ_DIR)/ambulance $(OBJ_DIR)/traffic_controller
	@mkdir -p $(OBJ_DIR)/hospital_dashboard $(OBJ_DIR)/traffic_dashboard
	@mkdir -p $(OBJ_DIR)/apps $(OBJ_DIR)/tests/unit

$(OBJ_DIR)/%.o: src/%.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(STATIC_LIB): $(ALL_OBJS)
	@echo "[AR] $@"
	@ar rcs $@ $(ALL_OBJS)

$(BIN_DIR)/aegis_coordinator: src/apps/coordinator_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BIN_DIR)/aegis_ambulance: src/apps/ambulance_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BIN_DIR)/aegis_traffic_controller: src/apps/traffic_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(SMOKE_TEST_BIN): tests/unit/test_build_smoke.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(FOUNDATION_TEST_BIN): tests/unit/test_common_foundation.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COMM_TEST_BIN): tests/unit/test_communication_infrastructure.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_TEST_BIN): tests/unit/test_coordinator_core.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(AMB_TEST_BIN): tests/unit/test_ambulance_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_TEST_BIN): tests/unit/test_traffic_signal_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

test: $(SMOKE_TEST_BIN) $(FOUNDATION_TEST_BIN) $(COMM_TEST_BIN) $(COORD_TEST_BIN) $(AMB_TEST_BIN) $(TRAFFIC_TEST_BIN)
	@echo "=========================================================="
	@echo " Running SMART AEGIS Test Suites"
	@echo "=========================================================="
	@$(SMOKE_TEST_BIN)
	@$(FOUNDATION_TEST_BIN)
	@$(COMM_TEST_BIN)
	@$(COORD_TEST_BIN)
	@$(AMB_TEST_BIN)
	@$(TRAFFIC_TEST_BIN)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts."
