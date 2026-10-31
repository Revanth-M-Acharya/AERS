/**
 * @file OledV2XDisplay.cpp
 * @brief Implementation of OLED V2X Display simulator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/OledV2XDisplay.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::hardware_integration::demo {

OledV2XDisplay::OledV2XDisplay(uint32_t width, uint32_t height)
    : width_(width),
      height_(height) {
    currentLines_ = {"", "SMART AEGIS V2X", "READY", ""};
}

bool OledV2XDisplay::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::IDLE;
    currentLines_ = {"[ V2X ADVISORY ]", "SMART AEGIS RTOS", "SYSTEM STANDBY", "READY"};
    updateCount_ = 1;
    return true;
}

void OledV2XDisplay::ShowAmbulanceApproaching() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::AMBULANCE_APPROACHING;
    currentLines_ = {
        "AMBULANCE",
        "APPROACHING",
        "",
        "MOVE LEFT"
    };
    updateCount_++;
}

void OledV2XDisplay::ShowGreenCorridorActive() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::GREEN_CORRIDOR_ACTIVE;
    currentLines_ = {
        "GREEN",
        "CORRIDOR",
        "ACTIVE",
        "DO NOT BLOCK"
    };
    updateCount_++;
}

void OledV2XDisplay::ShowMissionComplete() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::MISSION_COMPLETE;
    currentLines_ = {
        "MISSION",
        "COMPLETE",
        "",
        "SIGNALS NORMAL"
    };
    updateCount_++;
}

void OledV2XDisplay::ShowPoliceReplanning(const std::string& detourInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::POLICE_REPLANNING;
    currentLines_ = {
        "POLICE BARRICADE",
        "ROAD CLOSED: INT-2",
        "ROUTE REPLANNED",
        detourInfo
    };
    updateCount_++;
}

void OledV2XDisplay::ShowPriorityArbitration(const std::string& winningAmbulance, const std::string& heldAmbulance) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::PRIORITY_ARBITRATION;
    currentLines_ = {
        "PRIORITY SCHEDULER",
        "PASS: " + winningAmbulance,
        "HOLD: " + heldAmbulance,
        "EST WAIT: 15.0s"
    };
    updateCount_++;
}

void OledV2XDisplay::ShowCustom(const std::vector<std::string>& lines) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::CUSTOM;
    currentLines_ = lines;
    updateCount_++;
}

void OledV2XDisplay::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = V2XAdvisoryMode::IDLE;
    currentLines_ = {"", "", "", ""};
    updateCount_++;
}

std::string OledV2XDisplay::RenderAscii() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    const int frameWidth = 28;

    oss << "+" << std::string(frameWidth, '-') << "+\n";
    oss << "| " << std::left << std::setw(frameWidth - 2) << "[ V2X OLED DISPLAY ]" << " |\n";
    oss << "+" << std::string(frameWidth, '-') << "+\n";

    for (size_t i = 0; i < 4; ++i) {
        std::string line = (i < currentLines_.size()) ? currentLines_[i] : "";
        if (line.length() > static_cast<size_t>(frameWidth - 2)) {
            line = line.substr(0, frameWidth - 5) + "...";
        }
        oss << "| " << std::left << std::setw(frameWidth - 2) << line << " |\n";
    }

    oss << "+" << std::string(frameWidth, '-') << "+\n";
    return oss.str();
}

V2XAdvisoryMode OledV2XDisplay::GetActiveMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentMode_;
}

std::vector<std::string> OledV2XDisplay::GetCurrentLines() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentLines_;
}

uint64_t OledV2XDisplay::GetUpdateCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return updateCount_;
}

} // namespace aegis::hardware_integration::demo
