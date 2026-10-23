#include "aegis/common/uuid_utils.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace aegis {
namespace common {

std::string GenerateUuidV4() {
    static thread_local std::mt19937_64 rng([]() {
        std::random_device rd;
        auto seed = static_cast<uint64_t>(rd()) ^
                    static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        return seed;
    }());

    std::uniform_int_distribution<uint64_t> dist;
    uint64_t d1 = dist(rng);
    uint64_t d2 = dist(rng);

    // Set UUID v4 variant and version bits
    d1 = (d1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL; // Version 4
    d2 = (d2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // Variant 1 (RFC 4122)

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << (d1 >> 32) << "-"
        << std::setw(4) << ((d1 >> 16) & 0xFFFF) << "-"
        << std::setw(4) << (d1 & 0xFFFF) << "-"
        << std::setw(4) << (d2 >> 48) << "-"
        << std::setw(12) << (d2 & 0xFFFFFFFFFFFFULL);

    return oss.str();
}

std::string GenerateEventId(std::string_view prefix) {
    return std::string(prefix) + "_" + GenerateUuidV4();
}

std::string GenerateIncidentId() {
    return GenerateEventId("inc");
}

std::string FormatAmbulanceId(uint32_t unitNumber) {
    std::ostringstream oss;
    oss << "AMB-" << std::setfill('0') << std::setw(2) << unitNumber;
    return oss.str();
}

} // namespace common
} // namespace aegis
