# ==============================================================================
# SMART AEGIS: Universal Build Automation Makefile
# Provides seamless compilation on Linux/macOS using standard C++17 compilers
# (clang++ / g++) alongside CMake.
# ==============================================================================

CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
INCLUDES = -Iinclude -Isrc

BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Debug)
    CXXFLAGS += -g3 -O0 -DDEBUG
else
    CXXFLAGS += -O3 -DNDEBUG
endif

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj

# Library Sources
COMMON_SRCS = src/common/geo_utils.cpp
CONFIG_SRCS = src/config/config_manager.cpp
LOGGER_SRCS = src/logger/logger.cpp
SAFETY_SRCS = src/safety/watchdog.cpp src/safety/safety_monitor.cpp
NETWORK_SRCS = src/network/mqtt_client.cpp src/network/network_manager.cpp
SCHED_SRCS = src/priority_scheduler/priority_scheduler.cpp
ROUTE_SRCS = src/route_planner/route_planner.cpp
COORD_SRCS = src/coordinator/emergency_call_handler.cpp src/coordinator/dynamic_green_corridor.cpp src/coordinator/coordinator.cpp
AMB_SRCS = src/ambulance/gps_tracker.cpp src/ambulance/vitals_monitor.cpp src/ambulance/v2v_module.cpp src/ambulance/ambulance_node.cpp
TRAFFIC_SRCS = src/traffic_controller/gpio_interface.cpp src/traffic_controller/traffic_controller.cpp
HOSP_SRCS = src/hospital_dashboard/hospital_dashboard_backend.cpp
TRFDASH_SRCS = src/traffic_dashboard/traffic_dashboard_backend.cpp

ALL_SRCS = $(COMMON_SRCS) $(CONFIG_SRCS) $(LOGGER_SRCS) $(SAFETY_SRCS) $(NETWORK_SRCS) \
           $(SCHED_SRCS) $(ROUTE_SRCS) $(COORD_SRCS) $(AMB_SRCS) $(TRAFFIC_SRCS) \
           $(HOSP_SRCS) $(TRFDASH_SRCS)

ALL_OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SRCS))

STATIC_LIB = $(LIB_DIR)/libaegis.a

# App Executables
APPS = $(BIN_DIR)/aegis_coordinator $(BIN_DIR)/aegis_ambulance $(BIN_DIR)/aegis_traffic_controller
TEST_BIN = $(BIN_DIR)/test_build_smoke

.PHONY: all clean test directories info

all: directories $(STATIC_LIB) $(APPS) $(TEST_BIN)
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

$(TEST_BIN): tests/unit/test_build_smoke.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

test: $(TEST_BIN)
	@echo "=========================================================="
	@echo " Running SMART AEGIS Smoke & Build Verification Test"
	@echo "=========================================================="
	@$(TEST_BIN)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts."
