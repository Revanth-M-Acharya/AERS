/**
 * @file OledV2XDisplay.hpp
 * @brief SSD1306/I2C compatible OLED Display driver simulating V2X advisory messages.
 * Formats in-vehicle and roadside warnings during emergency corridor execution.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::hardware_integration::demo {

enum class V2XAdvisoryMode {
    IDLE,
    AMBULANCE_APPROACHING,
    GREEN_CORRIDOR_ACTIVE,
    MISSION_COMPLETE,
    POLICE_REPLANNING,
    PRIORITY_ARBITRATION,
    CUSTOM
};

class OledV2XDisplay {
public:
    OledV2XDisplay(uint32_t width = 128, uint32_t height = 64);
    ~OledV2XDisplay() = default;

    // Prevent copying
    OledV2XDisplay(const OledV2XDisplay&) = delete;
    OledV2XDisplay& operator=(const OledV2XDisplay&) = delete;

    /**
     * @brief Initialize display controller and clear frame buffer.
     */
    bool Initialize();

    /**
     * @brief Show standard "AMBULANCE APPROACHING / MOVE LEFT" V2X warning.
     */
    void ShowAmbulanceApproaching();

    /**
     * @brief Show "GREEN CORRIDOR ACTIVE" advisory message.
     */
    void ShowGreenCorridorActive();

    /**
     * @brief Show "MISSION COMPLETE" status message.
     */
    void ShowMissionComplete();

    /**
     * @brief Show "POLICE BARRICADE / ROAD CLOSED / ROUTE REPLANNED" advisory.
     */
    void ShowPoliceReplanning(const std::string& detourInfo = "BYPASS ACTIVE");

    /**
     * @brief Show priority conflict arbitration advisory at Intersection 2.
     */
    void ShowPriorityArbitration(const std::string& winningAmbulance, const std::string& heldAmbulance);

    /**
     * @brief Show arbitrary multi-line custom text message.
     */
    void ShowCustom(const std::vector<std::string>& lines);

    /**
     * @brief Clear display to blank state.
     */
    void Clear();

    /**
     * @brief Get rendered ASCII representation of current display screen.
     */
    std::string RenderAscii() const;

    /**
     * @brief Get active advisory mode.
     */
    V2XAdvisoryMode GetActiveMode() const;

    /**
     * @brief Get current active lines of text.
     */
    std::vector<std::string> GetCurrentLines() const;

    /**
     * @brief Total number of display updates rendered since boot.
     */
    uint64_t GetUpdateCount() const;

    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

private:
    uint32_t width_{128};
    uint32_t height_{64};

    mutable std::mutex mutex_;
    V2XAdvisoryMode currentMode_{V2XAdvisoryMode::IDLE};
    std::vector<std::string> currentLines_;
    uint64_t updateCount_{0};
};

} // namespace aegis::hardware_integration::demo
