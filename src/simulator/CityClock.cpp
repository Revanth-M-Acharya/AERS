/**
 * @file CityClock.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Deterministic City Simulation Clock Implementation.
 */

#include "CityClock.h"
#include <iomanip>
#include <sstream>
#include <cmath>

namespace aegis {
namespace simulator {

CityClock::CityClock(double startHour, double timeScale)
    : m_timeScale(timeScale)
    , m_currentHourOfDay(startHour)
{
}

void CityClock::SetTimeScale(double scale) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (scale <= 0.0) scale = 1.0;
    if (scale > 20.0) scale = 20.0;
    m_timeScale = scale;
}

double CityClock::GetTimeScale() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_timeScale;
}

void CityClock::SetSpeedMultiplier(SimulationSpeedMultiplier multiplier) {
    SetTimeScale(static_cast<double>(multiplier));
}

SimulationSpeedMultiplier CityClock::GetSpeedMultiplier() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int scale = static_cast<int>(std::round(m_timeScale));
    switch (scale) {
        case 2: return SimulationSpeedMultiplier::SPEED_2X;
        case 5: return SimulationSpeedMultiplier::SPEED_5X;
        case 10: return SimulationSpeedMultiplier::SPEED_10X;
        default: return SimulationSpeedMultiplier::SPEED_1X;
    }
}

void CityClock::Pause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPaused = true;
}

void CityClock::Resume() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPaused = false;
}

void CityClock::TogglePause() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPaused = !m_isPaused;
}

bool CityClock::IsPaused() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isPaused;
}

void CityClock::Reset(double startHour) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_elapsedSimSeconds = 0.0;
    m_currentHourOfDay = startHour;
    m_tickCount = 0;
    m_isPaused = false;
}

void CityClock::StepSingleTick(double dtSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_elapsedSimSeconds += dtSec;
    m_currentHourOfDay += (dtSec / 3600.0);
    if (m_currentHourOfDay >= 24.0) {
        m_currentHourOfDay = std::fmod(m_currentHourOfDay, 24.0);
    }
    m_tickCount++;
}

void CityClock::Advance(double wallDtSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isPaused) return;

    double simDtSec = wallDtSec * m_timeScale;
    m_elapsedSimSeconds += simDtSec;
    m_currentHourOfDay += (simDtSec / 3600.0);
    if (m_currentHourOfDay >= 24.0) {
        m_currentHourOfDay = std::fmod(m_currentHourOfDay, 24.0);
    }
    m_tickCount++;
}

double CityClock::GetElapsedSimSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_elapsedSimSeconds;
}

uint64_t CityClock::GetTickCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tickCount;
}

double CityClock::GetCurrentHourOfDay() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentHourOfDay;
}

std::string CityClock::GetTimeString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    double h = m_currentHourOfDay;
    int hours = static_cast<int>(h);
    double m = (h - static_cast<double>(hours)) * 60.0;
    int minutes = static_cast<int>(m);
    double s = (m - static_cast<double>(minutes)) * 60.0;
    int seconds = static_cast<int>(s);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << seconds;
    return oss.str();
}

std::string CityClock::GetIsoString() const {
    std::ostringstream oss;
    oss << "2026-10-29T" << GetTimeString() << "Z";
    return oss.str();
}

} // namespace simulator
} // namespace aegis

