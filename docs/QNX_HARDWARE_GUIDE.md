# SMART AEGIS (AERS) — Physical Hardware, Relay & GPIO Integration Guide
**Document ID:** `AEGIS-DOC-HW-20.6`  
**Target OS:** QNX Neutrino RTOS (SDP 8.0 / 7.1)  
**Target Hardware:** Raspberry Pi 4 Model B (Broadcom BCM2711 SoC)  
**Peripherals:** 4-Channel Optocoupled Active-Low Relays, Dual Traffic Signal Modules, Optical IR Sensors  

---

## 1. Hardware Architecture & Intersection Topology

Each roadside intersection unit is an autonomous cyber-physical edge station powered by a Raspberry Pi 4 Model B running QNX Neutrino RTOS. The board interfaces with two complementary physical traffic signal heads (Road A and Road B), an optocoupled relay board, and two optical IR vehicle detection sensors.

```
                           +--------------------------------------+
                           |  Raspberry Pi 4 Model B (aarch64le)  |
                           |  Broadcom BCM2711 SoC                |
                           |  QNX Neutrino RTOS (SDP 8.0 / 7.1)   |
                           +--------------------------------------+
                                      |               |
                         [GPIO Outputs]               [GPIO Inputs]
                                      |               |
             +------------------------+               +------------------------+
             |                                                                 |
             v                                                                 v
+-----------------------------+                                   +-------------------------+
| 4-Channel Relay (Active-Low)|                                   | Optical IR Sensors      |
| Ch 1: Road A Red   (GPIO 17)|                                   | Approach Sensor(GPIO 12)|
| Ch 2: Road A Yellow(GPIO 27)|                                   | Departure Sensor(GPIO 13)|
| Ch 3: Road A Green (GPIO 22)|                                   +-------------------------+
| Ch 4: Road B Green (GPIO 25)|
+-----------------------------+
             |
             v
+-----------------------------+
| Dual Traffic Signal Heads   |
| Road A: Red / Yellow / Green|
| Road B: Red / Yellow / Green|
+-----------------------------+
```

---

## 2. Raspberry Pi 4 BCM2711 Complete Pinout Assignment

| Physical Header Pin | BCM GPIO Number | Signal Assignment | Direction | Electrical Logic | Function |
|---|---|---|---|---|---|
| Pin 11 | **GPIO 17** | `ROAD_A_RED` | Output | Active-Low Relay (Ch 1) | Road A Stop Signal |
| Pin 13 | **GPIO 27** | `ROAD_A_YELLOW` | Output | Active-Low Relay (Ch 2) | Road A Warning Signal |
| Pin 15 | **GPIO 22** | `ROAD_A_GREEN` | Output | Active-Low Relay (Ch 3) | Road A Preemption / Go |
| Pin 16 | **GPIO 23** | `ROAD_B_RED` | Output | Direct 3.3V Logic | Road B Stop Signal |
| Pin 18 | **GPIO 24** | `ROAD_B_YELLOW` | Output | Direct 3.3V Logic | Road B Warning Signal |
| Pin 22 | **GPIO 25** | `ROAD_B_GREEN` | Output | Active-Low Relay (Ch 4) | Road B Go Signal |
| Pin 29 | **GPIO 5** | `PED_WALK` | Output | Direct 3.3V Logic | Pedestrian Walk Indicator |
| Pin 31 | **GPIO 6** | `BUZZER` | Output | Direct 3.3V Logic | Audible Preemption Alert |
| Pin 32 | **GPIO 12** | `IR_APPROACH` | Input | Pull-Up Active-Low | Ambulance Inbound Sensor |
| Pin 33 | **GPIO 13** | `IR_DEPARTURE`| Input | Pull-Up Active-Low | Ambulance Clearance Sensor |
| Pin 37 | **GPIO 26** | `STROBE` | Output | Direct 3.3V Logic | Optical Preemption Beacon |
| Pin 36 | **GPIO 16** | `MANUAL_OVERRIDE`| Input | Pull-Up Active-Low | Local Push-Button Override|

---

## 3. Optocoupled Relay Actuation Principle (Active-Low)

Industrial optocoupled relay modules protect the Raspberry Pi's Broadcom BCM2711 SoC by providing galvanic optical isolation between the low-voltage 3.3V GPIO pins and the 5V relay coils.

### Coil Actuation Logic
* **Coil Energized (Relay ON / Closed):** BCM2711 GPIO drives **`0V` (LOW)**. Current flows through the optocoupler LED, energizing the electromagnet and closing the contact.
* **Coil De-energized (Relay OFF / Open):** BCM2711 GPIO drives **`3.3V` (HIGH)**. Current stops flowing, turning off the optocoupler and releasing the armature.

### Power-On Safe State & Failure Recovery
1. During boot, unconfigured GPIO pins default to floating inputs. Pull-up resistors keep inputs high, preventing accidental relay closures.
2. When the `RelayController` initializes (`RelayController::Initialize`), it configures pins as outputs and drives them immediately to `HIGH` (`PinState::HIGH`), ensuring all relays are safely open.
3. Upon process termination or emergency shutdown (`RelayController::Shutdown`), `DeenergizeAll()` executes, driving all channels to `HIGH` and returning the intersection to its de-energized safe state.

---

## 4. Signal Sequencing & Complementary Dual-Road Logic

Safety requires that Road A and Road B operate complementarily to eliminate collision hazards:

```
[Phase 1: Road A Green / Road B Red] (Hold 22s)
               ↓
[Phase 2: Road A Yellow / Road B Red] (Clearance 4s)
               ↓
[Phase 3: ALL-RED CLEARANCE] (Hold 2s)
               ↓
[Phase 4: Road A Red / Road B Green] (Hold 22s)
               ↓
[Phase 5: Road A Red / Road B Yellow] (Clearance 4s)
               ↓
[Phase 6: ALL-RED CLEARANCE] (Hold 2s)
               ↓
        (Return to Phase 1)
```

### Safety Invariants
1. **Never Conflicting Green:** At no point can `Road A Green` and `Road B Green` be active simultaneously.
2. **Mandatory All-Red Hold:** Every phase transition between intersecting roads must include a minimum 2.0-second `ALL_RED_CLEARANCE` interval.
3. **Preemption Entry:** Preemption overrides normal cycle only after the conflicting green phase completes its yellow and all-red clearance.

---

## 5. Hardware Mutual Exclusion Safety Interlock

The `TrafficLightController` implements a deterministic safety interlock that runs before every output transition:

```cpp
bool TrafficLightController::EnforceMutualExclusion(LightColor roadA, LightColor roadB) {
    if (roadA == LightColor::GREEN && roadB == LightColor::GREEN) {
        m_conflictDetected = true;
        m_hardwareFault = true;
        // Trip emergency all-red clearance immediately
        m_roadAColor = LightColor::RED;
        m_roadBColor = LightColor::RED;
        ApplyHardwareOutputs();
        return false; // REJECT
    }
    return true; // ACCEPT
}
```

If any software fault or corrupt command attempts to activate simultaneous greens:
1. The transition is unconditionally rejected.
2. A hardware fault is tripped (`m_hardwareFault = true`).
3. Both roads are immediately forced to `RED` (`ALL_RED_CLEARANCE`).
4. The fault condition is logged to `logs/hardware.log` and broadcast to the Coordinator.

---

## 6. Optical IR Sensor Noise Filtering & Debouncing

Roadside infrared optical sensors are vulnerable to transient ambient lighting noise, vehicle vibration, and electrical spikes.

### The `OpticalSensorFilter` Algorithm
The filter operates on digital logic samples with two parameters:
* `debounceMs = 50`: Minimum continuous interruption time required to qualify a valid vehicle detection.
* `stuckTimeoutMs = 30000`: Maximum allowable continuous blockage before flagging a defective or blocked sensor.

```
Raw IR Signal:    --[ 15ms spike ]--       -----[ 65ms continuous beam break ]-----
Filtered State:   __________________       ____________[ QUALIFIED ARRIVAL ]_______
Action:           Spike Rejected           Emergency Preemption Triggered
```

1. **Noise Spikes (< 50 ms):** Discarded without state transition. `noiseSpikesFiltered` counter increments.
2. **Valid Arrival ($\ge 50$ ms):** Triggers qualified activation. Approach IR signals the preemption manager.
3. **Valid Departure ($\ge 50$ ms):** When the emergency vehicle crosses the departure beam, preemption is released automatically. The controller enters `ALL_RED_CLEARANCE` (2.0s) and restores the normal signal cycle.
4. **Stuck Sensor (> 30s):** If a sensor remains broken continuously for over 30 seconds, a `STUCK_SENSOR_FAULT` is raised, isolating the sensor to prevent locked preemption.

