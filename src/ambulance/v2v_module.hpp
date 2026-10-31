#ifndef AEGIS_AMBULANCE_V2V_MODULE_HPP
#define AEGIS_AMBULANCE_V2V_MODULE_HPP

#include <cstdint>

namespace aegis {
namespace ambulance {
#pragma once
#include <string>
class V2vModule {
public:
    V2vModule() = default;
    ~V2vModule() = default;

    bool Initialize(uint16_t udpPort);
    void BroadcastSafetyBeacon();
    void SendDirectPreemptionBeacon(const std::string& intersectionId);
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_V2V_MODULE_HPP
