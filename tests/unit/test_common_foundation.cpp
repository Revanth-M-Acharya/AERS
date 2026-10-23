#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

#include "aegis/common/types.hpp"
#include "aegis/common/constants.hpp"
#include "aegis/common/error_codes.hpp"
#include "aegis/common/error.hpp"
#include "aegis/common/result.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/uuid_utils.hpp"
#include "aegis/common/thread_safe_queue.hpp"
#include "aegis/common/circular_buffer.hpp"
#include "aegis/common/event.hpp"
#include "aegis/common/state_machine.hpp"
#include "aegis/common/timer.hpp"
#include "aegis/common/json_helper.hpp"
#include "aegis/common/serialization.hpp"
#include "aegis/common/validator.hpp"

// =============================================================================
// Derived Test Classes
// =============================================================================
class MockEmergencyEvent : public aegis::common::Event {
public:
    MockEmergencyEvent(const std::string& incidentId, double severity)
        : aegis::common::Event("MOCK_EMERGENCY"),
          m_incidentId(incidentId),
          m_severity(severity) {}

    std::string ToJson() const override {
        return aegis::common::Event::ToJson();
    }

    std::string m_incidentId;
    double m_severity;
};

enum class MockTrafficState {
    RED,
    YELLOW,
    GREEN
};

class MockTrafficFSM : public aegis::common::StateMachine<MockTrafficState> {
public:
    MockTrafficFSM() : aegis::common::StateMachine<MockTrafficState>(MockTrafficState::RED) {}

protected:
    bool CanTransition(MockTrafficState current, MockTrafficState next) const override {
        if (current == MockTrafficState::RED && next == MockTrafficState::GREEN) return true;
        if (current == MockTrafficState::GREEN && next == MockTrafficState::YELLOW) return true;
        if (current == MockTrafficState::YELLOW && next == MockTrafficState::RED) return true;
        return false;
    }
};

class MockTelemetryPayload : public aegis::common::ISerializable {
public:
    double speed{0.0};

    std::string SerializeToJson() const override {
        return "{\"speed\":" + std::to_string(speed) + "}";
    }

    aegis::common::ErrorCode DeserializeFromJson(std::string_view json) override {
        auto val = aegis::common::JsonHelper::ExtractDouble(json, "speed");
        if (val.has_value()) {
            speed = *val;
            return aegis::common::ErrorCode::SUCCESS;
        }
        return aegis::common::ErrorCode::ERR_INVALID_MESSAGE;
    }
};

// =============================================================================
// Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " Running SMART AEGIS Common Foundation Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Geo Utilities Test
    {
        std::cout << "[TEST] 1. Geo Utilities... ";
        aegis::common::GeoCoordinate a{12.971598, 77.594562, 0.0}; // Bangalore City
        aegis::common::GeoCoordinate b{12.973412, 77.597120, 0.0}; // ~340m away

        assert(aegis::common::IsValidCoordinate(a));
        assert(aegis::common::IsValidCoordinate(12.97, 77.59));
        assert(!aegis::common::IsValidCoordinate(95.0, 77.59)); // Invalid lat

        double dist = aegis::common::CalculateHaversineDistance(a, b);
        assert(dist > 300.0 && dist < 400.0);

        [[maybe_unused]] double bearing = aegis::common::CalculateBearing(a, b);
        assert(bearing >= 0.0 && bearing < 360.0);

        [[maybe_unused]] double eta = aegis::common::CalculateEtaSeconds(dist, 60.0); // 60 km/h
        assert(eta > 15.0 && eta < 30.0);
        assert(aegis::common::CalculateEtaSeconds(dist, 0.0) == 0.0);
        std::cout << "PASS" << std::endl;
    }

    // 2. Time Utilities Test
    {
        std::cout << "[TEST] 2. Time Utilities... ";
        [[maybe_unused]] uint64_t mono1 = aegis::common::GetMonotonicMicroseconds();
        [[maybe_unused]] uint64_t utc1 = aegis::common::GetUtcMilliseconds();
        assert(mono1 > 0);
        assert(utc1 > 1700000000000ULL); // After 2023

        std::string iso = aegis::common::FormatUtcIso8601(utc1);
        assert(!iso.empty());
        assert(iso.find("T") != std::string::npos);
        assert(iso.back() == 'Z');

        aegis::common::Stopwatch sw;
        assert(!sw.IsRunning());
        sw.Start();
        assert(sw.IsRunning());
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        sw.Stop();
        assert(!sw.IsRunning());
        assert(sw.ElapsedMicroseconds() >= 4000);
        assert(sw.ElapsedMilliseconds() >= 4);
        std::cout << "PASS" << std::endl;
    }

    // 3. UUID Utilities Test
    {
        std::cout << "[TEST] 3. UUID Utilities... ";
        std::string u1 = aegis::common::GenerateUuidV4();
        std::string u2 = aegis::common::GenerateUuidV4();
        assert(u1.size() == 36);
        assert(u1 != u2);
        assert(u1[14] == '4'); // UUID v4 version nibble

        std::string evtId = aegis::common::GenerateEventId("call");
        assert(evtId.rfind("call_", 0) == 0);

        std::string incId = aegis::common::GenerateIncidentId();
        assert(incId.rfind("inc_", 0) == 0);

        std::string ambId = aegis::common::FormatAmbulanceId(7);
        assert(ambId == "AMB-07");
        std::cout << "PASS" << std::endl;
    }

    // 4. Thread Safe Queue Test
    {
        std::cout << "[TEST] 4. Thread Safe Queue... ";
        aegis::common::ThreadSafeQueue<int> q;
        assert(q.Empty());
        assert(q.Size() == 0);

        q.Push(10);
        q.Push(20);
        assert(q.Size() == 2);
        assert(!q.Empty());

        int val = 0;
        assert(q.TryPop(val));
        assert(val == 10);
        assert(q.Size() == 1);

        [[maybe_unused]] bool timedSuccess = q.WaitForAndPop(val, std::chrono::milliseconds(10));
        assert(timedSuccess);
        assert(val == 20);
        assert(q.Empty());

        // Test timeout on empty queue
        [[maybe_unused]] bool timeoutResult = q.WaitForAndPop(val, std::chrono::milliseconds(5));
        assert(!timeoutResult);
        std::cout << "PASS" << std::endl;
    }

    // 5. Circular Ring Buffer Test
    {
        std::cout << "[TEST] 5. Circular Ring Buffer... ";
        aegis::common::CircularBuffer<int, 3> cb;
        assert(cb.IsEmpty());
        assert(!cb.IsFull());
        assert(cb.Size() == 0);

        assert(cb.Push(1));
        assert(cb.Push(2));
        assert(cb.Push(3));
        assert(cb.IsFull());
        assert(!cb.Push(4)); // Overflow rejected

        assert(cb.Peek() != nullptr && *cb.Peek() == 1);

        // Push with overwrite
        cb.PushOverwrite(4);
        assert(cb.IsFull());
        assert(*cb.Peek() == 2); // 1 was overwritten

        [[maybe_unused]] int popVal = 0;
        assert(cb.Pop(popVal) && popVal == 2);
        assert(cb.Pop(popVal) && popVal == 3);
        assert(cb.Pop(popVal) && popVal == 4);
        assert(cb.IsEmpty());
        assert(!cb.Pop(popVal));
        std::cout << "PASS" << std::endl;
    }

    // 6. Result<T> Utility Test
    {
        std::cout << "[TEST] 6. Result<T> Utility... ";
        [[maybe_unused]] auto okResult = aegis::common::Result<int>::Ok(42);
        assert(okResult.IsOk());
        assert(!okResult.IsErr());
        assert(okResult.Value() == 42);
        assert(okResult.ValueOr(0) == 42);

        [[maybe_unused]] auto errResult = aegis::common::Result<int>::Err(aegis::common::ErrorCode::ERR_GPS_TIMEOUT);
        assert(!errResult.IsOk());
        assert(errResult.IsErr());
        assert(errResult.Error() == aegis::common::ErrorCode::ERR_GPS_TIMEOUT);
        assert(errResult.ValueOr(99) == 99);

        [[maybe_unused]] auto voidOk = aegis::common::Result<void>::Ok();
        assert(voidOk.IsOk());
        [[maybe_unused]] auto voidErr = aegis::common::Result<void>::Err(aegis::common::ErrorCode::ERR_MQTT_DISCONNECTED);
        assert(voidErr.IsErr());
        assert(voidErr.Error() == aegis::common::ErrorCode::ERR_MQTT_DISCONNECTED);
        std::cout << "PASS" << std::endl;
    }

    // 7. Error Class Test
    {
        std::cout << "[TEST] 7. Error Class... ";
        aegis::common::AegisError err(
            aegis::common::ErrorCode::ERR_TRAFFIC_OFFLINE,
            aegis::common::ErrorSeverity::ERROR,
            "CORRIDOR",
            "Target intersection unresponsive");

        assert(err.Code() == aegis::common::ErrorCode::ERR_TRAFFIC_OFFLINE);
        assert(err.Severity() == aegis::common::ErrorSeverity::ERROR);
        assert(err.OriginModule() == "CORRIDOR");
        assert(err.TimestampMs() > 0);
        assert(!err.ToString().empty());
        std::cout << "PASS" << std::endl;
    }

    // 8. Event Base Class Test
    {
        std::cout << "[TEST] 8. Event Base Class... ";
        MockEmergencyEvent ev("INC-1092", 95.5);
        assert(ev.EventType() == "MOCK_EMERGENCY");
        assert(!ev.EventId().empty());
        assert(ev.TimestampMs() > 0);
        assert(ev.m_incidentId == "INC-1092");
        assert(ev.ToJson().find("MOCK_EMERGENCY") != std::string::npos);
        std::cout << "PASS" << std::endl;
    }

    // 9. State Machine Base Class Test
    {
        std::cout << "[TEST] 9. State Machine Base Class... ";
        MockTrafficFSM fsm;
        assert(fsm.GetCurrentState() == MockTrafficState::RED);

        // Legal transition: RED -> GREEN
        assert(fsm.TransitionTo(MockTrafficState::GREEN));
        assert(fsm.GetCurrentState() == MockTrafficState::GREEN);

        // Illegal transition: GREEN -> RED (must pass yellow)
        assert(!fsm.TransitionTo(MockTrafficState::RED));
        assert(fsm.GetCurrentState() == MockTrafficState::GREEN);

        // Legal: GREEN -> YELLOW -> RED
        assert(fsm.TransitionTo(MockTrafficState::YELLOW));
        assert(fsm.TransitionTo(MockTrafficState::RED));
        assert(fsm.GetCurrentState() == MockTrafficState::RED);
        std::cout << "PASS" << std::endl;
    }

    // 10. Timer Utilities Test
    {
        std::cout << "[TEST] 10. Timer Utilities... ";
        aegis::common::SoftwareTimer timer(15);
        assert(!timer.IsActive());
        assert(!timer.IsExpired());

        timer.Start();
        assert(timer.IsActive());
        assert(!timer.IsExpired());

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        assert(timer.IsExpired());
        assert(timer.RemainingMs() == 0);

        timer.Reset();
        assert(!timer.IsExpired());
        timer.Stop();
        assert(!timer.IsActive());
        std::cout << "PASS" << std::endl;
    }

    // 11. JSON Helper Wrapper Test
    {
        std::cout << "[TEST] 11. JSON Helper Wrapper... ";
        std::string rawJson = "{\"unitId\":\"AMB-01\",\"speed\":65.5,\"active\":true,\"seq\":1402}";
        [[maybe_unused]] auto unitId = aegis::common::JsonHelper::ExtractString(rawJson, "unitId");
        [[maybe_unused]] auto speed = aegis::common::JsonHelper::ExtractDouble(rawJson, "speed");
        [[maybe_unused]] auto active = aegis::common::JsonHelper::ExtractBool(rawJson, "active");
        [[maybe_unused]] auto seq = aegis::common::JsonHelper::ExtractInt64(rawJson, "seq");

        assert(unitId.has_value() && *unitId == "AMB-01");
        assert(speed.has_value() && *speed > 65.0 && *speed < 66.0);
        assert(active.has_value() && *active == true);
        assert(seq.has_value() && *seq == 1402);

        std::string esc = aegis::common::JsonHelper::EscapeString("hello \"world\" \n test");
        assert(esc.find("\\\"") != std::string::npos);
        std::cout << "PASS" << std::endl;
    }

    // 12. Common Constants Test
    {
        std::cout << "[TEST] 12. Common Constants... ";
        assert(aegis::common::constants::HEARTBEAT_INTERVAL_MS == 200);
        assert(aegis::common::constants::WATCHDOG_TIMEOUT_MS == 500);
        assert(aegis::common::constants::YELLOW_CLEARANCE_MIN_MS == 3000);
        assert(aegis::common::constants::V2V_UDP_PORT == 8999);
        std::cout << "PASS" << std::endl;
    }

    // 13. Strongly Typed Enums Test
    {
        std::cout << "[TEST] 13. Strongly Typed Enums... ";
        [[maybe_unused]] aegis::common::AmbulanceState ambState = aegis::common::AmbulanceState::EN_ROUTE_HOSPITAL;
        [[maybe_unused]] aegis::common::SignalPhase phase = aegis::common::SignalPhase::GREEN;
        [[maybe_unused]] aegis::common::TriageCategory triage = aegis::common::TriageCategory::CODE_RED;

        assert(ambState == aegis::common::AmbulanceState::EN_ROUTE_HOSPITAL);
        assert(phase == aegis::common::SignalPhase::GREEN);
        assert(triage == aegis::common::TriageCategory::CODE_RED);
        std::cout << "PASS" << std::endl;
    }

    // 14. Serialization Helper Interface Test
    {
        std::cout << "[TEST] 14. Serialization Helper Interface... ";
        MockTelemetryPayload payload;
        payload.speed = 88.5;

        std::string json = payload.SerializeToJson();
        assert(json.find("88.5") != std::string::npos);

        MockTelemetryPayload target;
        [[maybe_unused]] auto res = target.DeserializeFromJson(json);
        assert(res == aegis::common::ErrorCode::SUCCESS);
        assert(target.speed > 88.0 && target.speed < 89.0);
        std::cout << "PASS" << std::endl;
    }

    // 15. Validation Utilities Test
    {
        std::cout << "[TEST] 15. Validation Utilities... ";
        assert(aegis::common::Validator::IsInRange(50, 0, 100));
        assert(!aegis::common::Validator::IsInRange(150, 0, 100));

        assert(aegis::common::Validator::IsValidString("AMB-01", 3, 10));
        assert(!aegis::common::Validator::IsValidString("A", 3, 10));

        [[maybe_unused]] uint32_t crc1 = aegis::common::Validator::ComputeCrc32("123456789");
        assert(crc1 == 0xCBF43926); // Standard CRC-32 test vector
        std::cout << "PASS" << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " [SUCCESS] ALL 15 COMMON FOUNDATION COMPONENTS VERIFIED!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
