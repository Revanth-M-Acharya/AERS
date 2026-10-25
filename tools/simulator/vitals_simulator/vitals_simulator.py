#!/usr/bin/env python3
"""
Patient Vitals Simulator
Generates synthetic physiological telemetry at 1 Hz and triggers acute clinical events.
Outputs ICD Section 7.3 VitalsUpdate JSON payloads.
"""

import sys
import time
import json
import random
import argparse
from datetime import datetime, timezone

def generate_vitals(profile: str = "normal", unit_id: str = "AMB-01", patient_id: str = "P-90182-C") -> dict:
    now_ms = int(datetime.now(timezone.utc).timestamp() * 1000)

    if profile == "cardiac":
        # Cardiac arrest / asystole
        return {
            "unitId": unit_id,
            "patientId": patient_id,
            "timestamp": now_ms,
            "heartRate": 0,
            "spO2": random.randint(80, 86),
            "systolicBP": 0,
            "diastolicBP": 0,
            "respirationRate": 0,
            "temperatureCelsius": 36.2,
            "gcsScore": 3,
            "alert": "CARDIAC_ARREST"
        }
    elif profile == "hypoxia":
        # Acute respiratory failure
        return {
            "unitId": unit_id,
            "patientId": patient_id,
            "timestamp": now_ms,
            "heartRate": random.randint(115, 130),
            "spO2": random.randint(80, 85),
            "systolicBP": random.randint(105, 120),
            "diastolicBP": random.randint(70, 80),
            "respirationRate": random.randint(26, 32),
            "temperatureCelsius": 37.1,
            "gcsScore": 13,
            "alert": "LOW_SPO2"
        }
    elif profile == "shock":
        # Hypotensive shock
        return {
            "unitId": unit_id,
            "patientId": patient_id,
            "timestamp": now_ms,
            "heartRate": random.randint(125, 140),
            "spO2": random.randint(90, 93),
            "systolicBP": random.randint(68, 76),
            "diastolicBP": random.randint(40, 48),
            "respirationRate": random.randint(22, 26),
            "temperatureCelsius": 35.8,
            "gcsScore": 11,
            "alert": "CRITICAL_BP"
        }
    elif profile == "fever":
        # Severe pyrexia / sepsis
        return {
            "unitId": unit_id,
            "patientId": patient_id,
            "timestamp": now_ms,
            "heartRate": random.randint(110, 125),
            "spO2": random.randint(94, 96),
            "systolicBP": random.randint(95, 110),
            "diastolicBP": random.randint(58, 68),
            "respirationRate": random.randint(20, 24),
            "temperatureCelsius": round(random.uniform(39.6, 40.4), 1),
            "gcsScore": 14,
            "alert": "HIGH_TEMPERATURE"
        }
    else:
        # Normal physiological baseline
        return {
            "unitId": unit_id,
            "patientId": patient_id,
            "timestamp": now_ms,
            "heartRate": random.randint(72, 78),
            "spO2": random.randint(98, 99),
            "systolicBP": random.randint(118, 124),
            "diastolicBP": random.randint(78, 82),
            "respirationRate": random.randint(15, 17),
            "temperatureCelsius": round(random.uniform(36.8, 37.2), 1),
            "gcsScore": 15,
            "alert": "NONE"
        }

def main():
    parser = argparse.ArgumentParser(description="SMART AEGIS Biomedical Vitals Simulator")
    parser.add_argument("--profile", choices=["normal", "hypoxia", "cardiac", "shock", "fever"], default="normal", help="Physiological condition")
    parser.add_argument("--unit-id", default="AMB-01", help="Ambulance unit ID")
    parser.add_argument("--patient-id", default="P-90182-C", help="Patient tracking ID")
    parser.add_argument("--count", type=int, default=5, help="Number of telemetry samples")
    parser.add_argument("--continuous", action="store_true", help="Stream continuously at 1 Hz")
    args = parser.parse_args()

    print(f"[VITALS SIMULATOR] Profile: {args.profile.upper()} | Asset: {args.unit_id} | Patient: {args.patient_id}")

    emitted = 0
    try:
        while args.continuous or emitted < args.count:
            vitals = generate_vitals(args.profile, args.unit_id, args.patient_id)
            print(json.dumps(vitals))
            sys.stdout.flush()

            emitted += 1
            if args.continuous or emitted < args.count:
                time.sleep(1.0)
    except KeyboardInterrupt:
        print("\n[VITALS SIMULATOR] Terminated by user.")

if __name__ == "__main__":
    main()
