/**
 * @file priority_scheduler_main.cpp
 * @brief SMART AEGIS Clinical Priority Scheduler & IRSA Engine Executable.
 * Target Platform: QNX Neutrino RTOS 8.0 (Raspberry Pi 4) / POSIX Simulation
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "medical/PriorityEngine.h"
#include "medical/PatientProfile.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

void RenderCircularMeter(int score, const std::string& grade, const std::string& label) {
    std::cout << "\n           .---.           " << std::endl;
    std::cout << "        .-'     '-.        " << std::endl;
    std::cout << "       /     |     \\       " << std::endl;
    std::cout << "      |   " << std::setw(3) << score << "/100   |      " << std::endl;
    std::cout << "      |   [" << grade << "]   |      " << std::endl;
    std::cout << "       \\     |     /       " << std::endl;
    std::cout << "        '-.     .-'        " << std::endl;
    std::cout << "           '---'           " << std::endl;
    std::cout << "      >>> " << label << " <<< \n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string scenario = "Cardiac Arrest";
    bool jsonOutput = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--scenario" || arg == "-s") && i + 1 < argc) {
            scenario = argv[++i];
        } else if (arg == "--json") {
            jsonOutput = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: priority_scheduler [OPTIONS]\n"
                      << "  --scenario, -s <name>   Preset scenario (Cardiac Arrest, Stroke, Trauma, Burn, Poisoning, Normal)\n"
                      << "  --json                  Output JSON telemetry\n"
                      << "  --help, -h              Display this help text\n";
            return 0;
        }
    }

    // Evaluate clinical score based on scenario
    int score = 45;
    std::string grade = "P3";
    std::string label = "P3 MEDIUM PRIORITY";
    std::string hospital = "City General Hospital";
    std::string specialty = "Emergency Medicine & Acute Care";
    int corridorLen = 4;
    int holdSec = 30;

    if (scenario == "Cardiac Arrest" || scenario == "cardiac") {
        score = 98;
        grade = "P1";
        label = "P1 CRITICAL (CODE RED)";
        hospital = "Metro Heart & Vascular Institute";
        specialty = "Cardiac Cath Lab & ECMO Resuscitation";
        corridorLen = 8;
        holdSec = 60;
    } else if (scenario == "Stroke" || scenario == "stroke") {
        score = 88;
        grade = "P1";
        label = "P1 CRITICAL (CODE RED)";
        hospital = "St. Jude Comprehensive Stroke Center";
        specialty = "Endovascular Thrombectomy & Neuro-ICU";
        corridorLen = 7;
        holdSec = 60;
    } else if (scenario == "Trauma" || scenario == "accident") {
        score = 92;
        grade = "P1";
        label = "P1 CRITICAL (CODE RED)";
        hospital = "City Trauma Center";
        specialty = "Level 1 Multi-Trauma Surgical Bay";
        corridorLen = 8;
        holdSec = 60;
    } else if (scenario == "Burn" || scenario == "fire") {
        score = 78;
        grade = "P1";
        label = "P1 CRITICAL (CODE RED)";
        hospital = "City General Burn Center";
        specialty = "Hyperbaric Critical Burn Unit";
        corridorLen = 6;
        holdSec = 45;
    } else if (scenario == "Poisoning" || scenario == "toxic") {
        score = 65;
        grade = "P2";
        label = "P2 HIGH PRIORITY (CODE YELLOW)";
        hospital = "Central Toxicology & Poison Center";
        specialty = "Antidote Repository & Dialysis Unit";
        corridorLen = 5;
        holdSec = 45;
    } else if (scenario == "Normal" || scenario == "transport") {
        score = 15;
        grade = "P4";
        label = "P4 LOW PRIORITY (ROUTINE)";
        hospital = "City General Clinic";
        specialty = "Outpatient Triage";
        corridorLen = 2;
        holdSec = 15;
    }

    if (jsonOutput) {
        std::cout << "{\n"
                  << "  \"scenario\": \"" << scenario << "\",\n"
                  << "  \"priority_score\": " << score << ",\n"
                  << "  \"priority_grade\": \"" << grade << "\",\n"
                  << "  \"priority_label\": \"" << label << "\",\n"
                  << "  \"hospital_recommendation\": \"" << hospital << "\",\n"
                  << "  \"corridor_intersections\": " << corridorLen << ",\n"
                  << "  \"max_signal_hold_sec\": " << holdSec << "\n"
                  << "}\n";
        return 0;
    }

    std::cout << "================================================================================" << std::endl;
    std::cout << " SMART AEGIS: INTELLIGENT PRIORITY SCHEDULER & CLINICAL IRSA ENGINE" << std::endl;
    std::cout << " Evaluated Scenario: " << scenario << std::endl;
    std::cout << "================================================================================" << std::endl;

    RenderCircularMeter(score, grade, label);

    std::cout << "========================== IRSA AUTONOMOUS ACTION PLAN ==========================" << std::endl;
    std::cout << "  Recommended Hospital : " << hospital << std::endl;
    std::cout << "  Specialty Staging    : " << specialty << std::endl;
    std::cout << "  Green Corridor Scope : " << corridorLen << " Intersections (Continuous Preemption)" << std::endl;
    std::cout << "  Max Signal Hold Time : " << holdSec << " Seconds (All-Red Conflict Interlock)" << std::endl;
    std::cout << "  Police Escort        : " << (score >= 80 ? "MANDATORY CRUISER LEAD" : "ADVISORY") << std::endl;
    std::cout << "  Traffic Aggressive   : " << score << "% Signal Priority" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
