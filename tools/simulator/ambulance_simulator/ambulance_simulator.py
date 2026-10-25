#!/usr/bin/env python3
"""
Ambulance Simulator
Simulates vehicle state machine progression, kinematics generation, and complete mission lifecycle:
Dispatch -> Travel to patient -> Pickup -> Travel hospital -> Arrive -> Mission complete
"""

import sys
import time
import math
import json
import argparse
from datetime import datetime, timezone

class AmbulanceMissionSim:
    def __init__(self, unit_id="AMB-01", incident_id="INC-1001"):
        self.unit_id = unit_id
        self.incident_id = incident_id
        self.state = "AVAILABLE"
        self.speed = 0.0
        self.heading = 0.0
        
        # Coordinates (Bangalore Metropolitan Demo)
        self.station_pos = (12.971598, 77.594562)
        self.patient_pos = (12.976000, 77.601000)
        self.hospital_pos = (12.982000, 77.610000)
        self.current_pos = list(self.station_pos)

    def calculate_bearing(self, p1, p2):
        lat1, lon1 = math.radians(p1[0]), math.radians(p1[1])
        lat2, lon2 = math.radians(p2[0]), math.radians(p2[1])
        dlon = lon2 - lon1
        y = math.sin(dlon) * math.cos(lat2)
        x = math.cos(lat1) * math.sin(lat2) - math.sin(lat1) * math.cos(lat2) * math.cos(dlon)
        deg = math.degrees(math.atan2(y, x))
        return (deg + 360.0) % 360.0

    def calculate_dist_meters(self, p1, p2):
        r = 6371000.0
        lat1, lon1 = math.radians(p1[0]), math.radians(p1[1])
        lat2, lon2 = math.radians(p2[0]), math.radians(p2[1])
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        a = math.sin(dlat / 2.0)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2.0)**2
        return 2 * r * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    def move_towards(self, target, speed_kmh, dt_sec):
        dist = self.calculate_dist_meters(self.current_pos, target)
        travel_m = (speed_kmh / 3.6) * dt_sec
        self.heading = self.calculate_bearing(self.current_pos, target)
        self.speed = speed_kmh

        if dist <= travel_m or dist < 5.0:
            self.current_pos = list(target)
            return True # Reached
        else:
            rad = math.radians(self.heading)
            dlat = (travel_m * math.cos(rad)) / 6371000.0 * (180.0 / math.pi)
            dlon = (travel_m * math.sin(rad)) / (6371000.0 * math.cos(math.radians(self.current_pos[0]))) * (180.0 / math.pi)
            self.current_pos[0] += dlat
            self.current_pos[1] += dlon
            return False

    def emit_telemetry(self):
        now_ms = int(datetime.now(timezone.utc).timestamp() * 1000)
        telemetry = {
            "unitId": self.unit_id,
            "incidentId": self.incident_id,
            "state": self.state,
            "timestamp": now_ms,
            "latitude": round(self.current_pos[0], 6),
            "longitude": round(self.current_pos[1], 6),
            "speed": round(self.speed, 1),
            "heading": round(self.heading, 1)
        }
        print(json.dumps(telemetry))
        sys.stdout.flush()

    def run_mission(self, step_sec=1.0):
        print(f"[MISSION SIMULATOR] Launching mission for {self.unit_id} (Incident: {self.incident_id})")
        
        # Step 1: Dispatch
        self.state = "DISPATCHED"
        self.emit_telemetry()
        time.sleep(step_sec)

        # Step 2: Travel to patient
        self.state = "EN_ROUTE_PATIENT"
        print("[MISSION SIMULATOR] Phase: EN_ROUTE_PATIENT")
        while not self.move_towards(self.patient_pos, speed_kmh=65.0, dt_sec=step_sec * 2.0):
            self.emit_telemetry()
            time.sleep(step_sec)

        # Step 3: Pickup on-scene
        self.state = "ON_SCENE"
        self.speed = 0.0
        print("[MISSION SIMULATOR] Phase: ON_SCENE (Stabilizing Patient)")
        self.emit_telemetry()
        time.sleep(step_sec * 2.0)

        # Step 4: Travel to hospital
        self.state = "EN_ROUTE_HOSPITAL"
        print("[MISSION SIMULATOR] Phase: EN_ROUTE_HOSPITAL")
        while not self.move_towards(self.hospital_pos, speed_kmh=70.0, dt_sec=step_sec * 2.0):
            self.emit_telemetry()
            time.sleep(step_sec)

        # Step 5: Arrive at Hospital
        self.state = "ARRIVED"
        self.speed = 0.0
        print("[MISSION SIMULATOR] Phase: ARRIVED (Hospital Emergency Department)")
        self.emit_telemetry()
        time.sleep(step_sec * 2.0)

        # Step 6: Mission Complete
        self.state = "AVAILABLE"
        print("[MISSION SIMULATOR] Phase: MISSION_COMPLETE (Unit Restored to Fleet)")
        self.emit_telemetry()

def main():
    parser = argparse.ArgumentParser(description="SMART AEGIS Ambulance Mission Simulator")
    parser.add_argument("--unit-id", default="AMB-01", help="Ambulance asset ID")
    parser.add_argument("--incident-id", default="INC-1001", help="Incident ID")
    parser.add_argument("--step-sec", type=float, default=0.5, help="Simulation step interval in seconds")
    args = parser.parse_args()

    sim = AmbulanceMissionSim(args.unit_id, args.incident_id)
    sim.run_mission(step_sec=args.step_sec)

if __name__ == "__main__":
    main()
