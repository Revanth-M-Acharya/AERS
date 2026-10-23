#ifndef AEGIS_EDGE_V2V_MODULE_HPP
#define AEGIS_EDGE_V2V_MODULE_HPP

#include <string>
#include <cstdint>

namespace aegis {
namespace edge {
namespace ambulance {

/**
 * @brief Autonomous ad-hoc wireless transceiver for direct V2V BSMs and V2I fallback beacons.
 * TODO: Implement UDP broadcast and direct preemption frame dispatch.
 */
class V2vModule {
public:
    V2vModule();
    ~V2vModule();

    bool Initialize(uint16_t udpPort);
    void BroadcastSafetyBeacon();
    void SendDirectPreemptionBeacon(const std::string& intersectionId);
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_V2V_MODULE_HPP
