/**
 * @file irsaEngine.ts
 * @brief Intelligent Risk & Severity Assessment (IRSA) Engine.
 * Multi-criteria weighted mathematical scoring computing patient severity,
 * environmental risk, route complexity, hospital availability, and dynamic preemption actions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import { PatientCase, IRSAResult, PriorityGrade } from '../types';

export const DEFAULT_PATIENT_CASE: PatientCase = {
  patientName: 'Jane Doe',
  age: 48,
  gender: 'Female',
  weightKg: 68,
  heartRate: 112,
  bloodPressureSystolic: 155,
  bloodPressureDiastolic: 95,
  spO2: 91,
  respirationRate: 24,
  temperatureC: 37.4,
  gcsScore: 14,
  painLevel: 7,
  bloodLoss: 'None',
  isPregnant: false,
  hasTrauma: false,
  hasBurn: false,
  hasStroke: false,
  hasHeartAttack: true,
  hasCardiacArrest: false,
  hasPoisoning: false,
  hasAccident: false,
  hasHeadInjury: false,
  hasMultipleFractures: false,
  hasInternalBleeding: false,
  isDiabetic: false,
  hasAsthma: false,
  hasSeizure: false,
  hasCancer: false,
  ventilatorRequired: false,
  oxygenRequired: true,
  icuRequired: true,
  estimatedDistanceKm: 4.8,
  trafficDensity: 'Heavy',
  weather: 'Clear',
  manualOverride: false,
  policeEscortRequired: false,
  vipConflict: false,
};

export const PREDEFINED_SCENARIOS: Record<string, PatientCase> = {
  'Heart Attack': {
    patientName: 'Robert Vance',
    age: 58,
    gender: 'Male',
    weightKg: 84,
    heartRate: 128,
    bloodPressureSystolic: 170,
    bloodPressureDiastolic: 105,
    spO2: 89,
    respirationRate: 26,
    temperatureC: 37.1,
    gcsScore: 13,
    painLevel: 9,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: true,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: true,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 5.2,
    trafficDensity: 'Heavy',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  'Cardiac Arrest': {
    patientName: 'Arthur Pendelton',
    age: 64,
    gender: 'Male',
    weightKg: 79,
    heartRate: 0,
    bloodPressureSystolic: 45,
    bloodPressureDiastolic: 20,
    spO2: 68,
    respirationRate: 4,
    temperatureC: 35.8,
    gcsScore: 3,
    painLevel: 10,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: true,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 4.1,
    trafficDensity: 'Gridlock',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  Stroke: {
    patientName: 'Eleanor Campbell',
    age: 72,
    gender: 'Female',
    weightKg: 62,
    heartRate: 104,
    bloodPressureSystolic: 195,
    bloodPressureDiastolic: 115,
    spO2: 92,
    respirationRate: 20,
    temperatureC: 37.2,
    gcsScore: 10,
    painLevel: 6,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: true,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: true,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 6.4,
    trafficDensity: 'Moderate',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  'Road Accident': {
    patientName: 'Marcus Miller',
    age: 29,
    gender: 'Male',
    weightKg: 76,
    heartRate: 136,
    bloodPressureSystolic: 85,
    bloodPressureDiastolic: 52,
    spO2: 88,
    respirationRate: 32,
    temperatureC: 36.4,
    gcsScore: 8,
    painLevel: 10,
    bloodLoss: 'Severe',
    isPregnant: false,
    hasTrauma: true,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: true,
    hasHeadInjury: true,
    hasMultipleFractures: true,
    hasInternalBleeding: true,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 3.5,
    trafficDensity: 'Gridlock',
    weather: 'Rain',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  'Fire Burn': {
    patientName: 'David Zhang',
    age: 34,
    gender: 'Male',
    weightKg: 72,
    heartRate: 130,
    bloodPressureSystolic: 100,
    bloodPressureDiastolic: 65,
    spO2: 86,
    respirationRate: 28,
    temperatureC: 38.6,
    gcsScore: 12,
    painLevel: 10,
    bloodLoss: 'Mild',
    isPregnant: false,
    hasTrauma: true,
    hasBurn: true,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: true,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 7.2,
    trafficDensity: 'Moderate',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: false,
    vipConflict: false,
  },
  'Mass Casualty': {
    patientName: 'MCI Victim Alpha-04',
    age: 38,
    gender: 'Other',
    weightKg: 70,
    heartRate: 142,
    bloodPressureSystolic: 80,
    bloodPressureDiastolic: 48,
    spO2: 84,
    respirationRate: 34,
    temperatureC: 36.1,
    gcsScore: 7,
    painLevel: 10,
    bloodLoss: 'Massive',
    isPregnant: false,
    hasTrauma: true,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: true,
    hasHeadInjury: true,
    hasMultipleFractures: true,
    hasInternalBleeding: true,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 2.8,
    trafficDensity: 'Gridlock',
    weather: 'Storm',
    manualOverride: true,
    policeEscortRequired: true,
    vipConflict: true,
  },
  'Pregnant Emergency': {
    patientName: 'Samantha Ross',
    age: 31,
    gender: 'Female',
    weightKg: 78,
    heartRate: 118,
    bloodPressureSystolic: 165,
    bloodPressureDiastolic: 110,
    spO2: 95,
    respirationRate: 22,
    temperatureC: 37.8,
    gcsScore: 14,
    painLevel: 9,
    bloodLoss: 'Moderate',
    isPregnant: true,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: true,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 4.5,
    trafficDensity: 'Moderate',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  Poisoning: {
    patientName: 'Vikram Patel',
    age: 26,
    gender: 'Male',
    weightKg: 65,
    heartRate: 52,
    bloodPressureSystolic: 88,
    bloodPressureDiastolic: 54,
    spO2: 89,
    respirationRate: 10,
    temperatureC: 35.9,
    gcsScore: 9,
    painLevel: 8,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: true,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: true,
    hasCancer: false,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 5.6,
    trafficDensity: 'Heavy',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: false,
    vipConflict: false,
  },
  'Asthma Attack': {
    patientName: 'Chloe Bennett',
    age: 19,
    gender: 'Female',
    weightKg: 55,
    heartRate: 132,
    bloodPressureSystolic: 135,
    bloodPressureDiastolic: 85,
    spO2: 82,
    respirationRate: 36,
    temperatureC: 37.0,
    gcsScore: 14,
    painLevel: 7,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: true,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: true,
    icuRequired: false,
    estimatedDistanceKm: 3.2,
    trafficDensity: 'Low',
    weather: 'Fog',
    manualOverride: false,
    policeEscortRequired: false,
    vipConflict: false,
  },
  'Child Emergency': {
    patientName: 'Leo Morales',
    age: 6,
    gender: 'Male',
    weightKg: 21,
    heartRate: 155,
    bloodPressureSystolic: 80,
    bloodPressureDiastolic: 45,
    spO2: 87,
    respirationRate: 42,
    temperatureC: 40.2,
    gcsScore: 11,
    painLevel: 8,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: true,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 4.0,
    trafficDensity: 'Moderate',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: true,
    vipConflict: false,
  },
  'Old Age Critical': {
    patientName: 'Henry Higgins',
    age: 86,
    gender: 'Male',
    weightKg: 64,
    heartRate: 116,
    bloodPressureSystolic: 82,
    bloodPressureDiastolic: 48,
    spO2: 85,
    respirationRate: 26,
    temperatureC: 38.9,
    gcsScore: 9,
    painLevel: 6,
    bloodLoss: 'Mild',
    isPregnant: false,
    hasTrauma: true,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: true,
    hasMultipleFractures: true,
    hasInternalBleeding: false,
    isDiabetic: true,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: true,
    ventilatorRequired: true,
    oxygenRequired: true,
    icuRequired: true,
    estimatedDistanceKm: 5.0,
    trafficDensity: 'Heavy',
    weather: 'Rain',
    manualOverride: false,
    policeEscortRequired: false,
    vipConflict: false,
  },
  'Normal Transport': {
    patientName: 'Alice Young',
    age: 41,
    gender: 'Female',
    weightKg: 65,
    heartRate: 74,
    bloodPressureSystolic: 122,
    bloodPressureDiastolic: 78,
    spO2: 98,
    respirationRate: 16,
    temperatureC: 36.8,
    gcsScore: 15,
    painLevel: 2,
    bloodLoss: 'None',
    isPregnant: false,
    hasTrauma: false,
    hasBurn: false,
    hasStroke: false,
    hasHeartAttack: false,
    hasCardiacArrest: false,
    hasPoisoning: false,
    hasAccident: false,
    hasHeadInjury: false,
    hasMultipleFractures: false,
    hasInternalBleeding: false,
    isDiabetic: false,
    hasAsthma: false,
    hasSeizure: false,
    hasCancer: false,
    ventilatorRequired: false,
    oxygenRequired: false,
    icuRequired: false,
    estimatedDistanceKm: 6.0,
    trafficDensity: 'Low',
    weather: 'Clear',
    manualOverride: false,
    policeEscortRequired: false,
    vipConflict: false,
  },
};

/**
 * Intelligent Risk & Severity Assessment (IRSA) Engine
 */
export function calculateIRSA(patient: PatientCase): IRSAResult {
  // 1. Compute Medical Severity (0 - 100)
  let medScore = 0;

  // Vitals scoring
  if (patient.hasCardiacArrest) {
    medScore += 95;
  } else {
    // Heart rate
    if (patient.heartRate > 140 || patient.heartRate < 45) medScore += 18;
    else if (patient.heartRate > 115 || patient.heartRate < 55) medScore += 12;
    else if (patient.heartRate > 100) medScore += 6;

    // Blood Pressure (MAP or Systolic deviation)
    if (patient.bloodPressureSystolic < 80 || patient.bloodPressureSystolic > 200) medScore += 20;
    else if (patient.bloodPressureSystolic < 90 || patient.bloodPressureSystolic > 180) medScore += 14;
    else if (patient.bloodPressureSystolic < 100 || patient.bloodPressureSystolic > 160) medScore += 8;

    // SpO2
    if (patient.spO2 < 80) medScore += 25;
    else if (patient.spO2 < 88) medScore += 18;
    else if (patient.spO2 < 93) medScore += 10;
    else if (patient.spO2 < 96) medScore += 4;

    // Respiration rate
    if (patient.respirationRate > 32 || patient.respirationRate < 8) medScore += 18;
    else if (patient.respirationRate > 24 || patient.respirationRate < 10) medScore += 10;

    // GCS Score (Glasgow Coma Scale: 3 to 15)
    if (patient.gcsScore <= 8) medScore += 25;
    else if (patient.gcsScore <= 12) medScore += 15;
    else if (patient.gcsScore <= 14) medScore += 6;

    // Temperature
    if (patient.temperatureC > 39.5 || patient.temperatureC < 35.0) medScore += 10;
    else if (patient.temperatureC > 38.5) medScore += 5;

    // Pain Level
    medScore += Math.round(patient.painLevel * 0.8);

    // Blood Loss
    if (patient.bloodLoss === 'Massive') medScore += 30;
    else if (patient.bloodLoss === 'Severe') medScore += 22;
    else if (patient.bloodLoss === 'Moderate') medScore += 12;
    else if (patient.bloodLoss === 'Mild') medScore += 5;

    // Acute Conditions
    if (patient.hasHeartAttack) medScore += 28;
    if (patient.hasStroke) medScore += 28;
    if (patient.hasInternalBleeding) medScore += 24;
    if (patient.hasHeadInjury) medScore += 20;
    if (patient.hasMultipleFractures) medScore += 16;
    if (patient.hasTrauma) medScore += 16;
    if (patient.hasBurn) medScore += 18;
    if (patient.hasPoisoning) medScore += 20;
    if (patient.hasAccident) medScore += 14;
    if (patient.hasSeizure) medScore += 16;
    if (patient.hasAsthma && patient.spO2 < 90) medScore += 18;
    if (patient.isPregnant && (patient.bloodLoss !== 'None' || patient.bloodPressureSystolic > 150)) medScore += 22;

    // Resource Requirements
    if (patient.ventilatorRequired) medScore += 25;
    if (patient.icuRequired) medScore += 18;
    if (patient.oxygenRequired) medScore += 8;
  }

  const medicalSeverity = Math.min(100, Math.max(5, medScore));

  // 2. Traffic Severity (0 - 100)
  let trafficSeverity = 15;
  if (patient.trafficDensity === 'Gridlock') trafficSeverity = 95;
  else if (patient.trafficDensity === 'Heavy') trafficSeverity = 70;
  else if (patient.trafficDensity === 'Moderate') trafficSeverity = 40;
  else trafficSeverity = 15;

  // 3. Environmental Severity (0 - 100)
  let environmentalSeverity = 10;
  if (patient.weather === 'Storm') environmentalSeverity = 85;
  else if (patient.weather === 'Rain') environmentalSeverity = 55;
  else if (patient.weather === 'Fog') environmentalSeverity = 65;
  else environmentalSeverity = 10;

  // 4. Route Complexity (0 - 100)
  const routeComplexity = Math.min(100, Math.round((patient.estimatedDistanceKm / 10) * 60 + trafficSeverity * 0.4));

  // 5. Hospital Availability & Match (0 - 100)
  const hospitalAvailability = patient.icuRequired ? 40 : 75; // 40% availability if ICU needed
  const vehicleReadiness = 96; // Fleet readiness
  const roadCongestion = trafficSeverity;
  const nodeFailures = 0; // Baseline healthy
  const signalAvailability = 100;
  const weatherImpact = environmentalSeverity;

  // 6. Weighted Priority Score Calculation
  // Priority Weighting:
  // Medical Severity: 60%
  // Traffic Severity: 15%
  // Route Complexity: 10%
  // Environmental: 10%
  // VIP / Police Factors: 5%
  let rawScore =
    medicalSeverity * 0.60 +
    trafficSeverity * 0.15 +
    routeComplexity * 0.10 +
    environmentalSeverity * 0.10 +
    (patient.policeEscortRequired || patient.vipConflict ? 5 : 0);

  // Manual Override guarantees P1 Critical
  if (patient.manualOverride) {
    rawScore = Math.max(rawScore, 98);
  }

  const priorityScore = Math.min(100, Math.max(1, Math.round(rawScore)));

  // Classify Priority Grade
  let priorityClass: PriorityGrade = 'P4_LOW';
  let priorityLabel: 'P1 Critical' | 'P2 High' | 'P3 Medium' | 'P4 Low' = 'P4 Low';
  let priorityColor = '#10B981'; // Green

  if (priorityScore >= 75) {
    priorityClass = 'P1_CRITICAL';
    priorityLabel = 'P1 Critical';
    priorityColor = '#EF4444'; // Red
  } else if (priorityScore >= 50) {
    priorityClass = 'P2_HIGH';
    priorityLabel = 'P2 High';
    priorityColor = '#F97316'; // Orange
  } else if (priorityScore >= 25) {
    priorityClass = 'P3_MEDIUM';
    priorityLabel = 'P3 Medium';
    priorityColor = '#EAB308'; // Yellow
  } else {
    priorityClass = 'P4_LOW';
    priorityLabel = 'P4 Low';
    priorityColor = '#10B981'; // Green
  }

  // 7. Hospital Recommendation Matching
  let targetHospital = {
    id: 'HOSP-01',
    name: 'City General Trauma Center',
    specialty: 'Level 1 Comprehensive Trauma & Acute Emergency',
    distanceKm: patient.estimatedDistanceKm,
    traumaBaysAvailable: 4,
    icuBedsAvailable: 8,
    erWaitMinutes: 0,
    suitabilityScore: 98,
    rationale: 'Primary Level 1 Trauma Facility with active Cath Lab, Neuro-ICU, and 4 trauma bays ready.',
  };

  if (patient.hasHeartAttack || patient.hasCardiacArrest) {
    targetHospital = {
      id: 'HOSP-02',
      name: 'Metro Heart & Vascular Institute',
      specialty: 'Cardiac Cath Lab, ECMO & Resuscitation Center',
      distanceKm: patient.estimatedDistanceKm + 0.6,
      traumaBaysAvailable: 3,
      icuBedsAvailable: 6,
      erWaitMinutes: 0,
      suitabilityScore: 99,
      rationale: 'Cath Lab Team Activated; Door-to-balloon pathway ready with on-call interventional cardiologist.',
    };
  } else if (patient.hasStroke) {
    targetHospital = {
      id: 'HOSP-03',
      name: 'St. Jude Comprehensive Stroke & Neurology Institute',
      specialty: 'Acute Stroke Interventional Radiology & Neuro-ICU',
      distanceKm: patient.estimatedDistanceKm + 1.1,
      traumaBaysAvailable: 5,
      icuBedsAvailable: 4,
      erWaitMinutes: 0,
      suitabilityScore: 97,
      rationale: 'Rapid CT perfusion scan cleared; tPA infusion suite and neuro-thrombectomy team staged.',
    };
  } else if (patient.hasBurn) {
    targetHospital = {
      id: 'HOSP-01',
      name: 'City General Specialized Burn Center',
      specialty: 'Dedicated Hyperbaric & Critical Burn ICU',
      distanceKm: patient.estimatedDistanceKm,
      traumaBaysAvailable: 2,
      icuBedsAvailable: 3,
      erWaitMinutes: 0,
      suitabilityScore: 96,
      rationale: 'Sterile thermal trauma isolation bay prepped with warm fluids and respiratory support team.',
    };
  } else if (patient.age < 14) {
    targetHospital = {
      id: 'HOSP-04',
      name: 'Apex Children’s & Pediatric Hospital',
      specialty: 'Pediatric Intensive Care Unit (PICU)',
      distanceKm: patient.estimatedDistanceKm + 0.4,
      traumaBaysAvailable: 6,
      icuBedsAvailable: 10,
      erWaitMinutes: 0,
      suitabilityScore: 98,
      rationale: 'Board-certified Pediatric Intensivists on stand-by; weight-adjusted medication pre-staged.',
    };
  } else if (patient.isPregnant) {
    targetHospital = {
      id: 'HOSP-05',
      name: 'Greenfield Maternity & Maternal-Fetal Center',
      specialty: 'High-Risk Obstetrics & Level IV NICU',
      distanceKm: patient.estimatedDistanceKm + 0.8,
      traumaBaysAvailable: 4,
      icuBedsAvailable: 5,
      erWaitMinutes: 0,
      suitabilityScore: 97,
      rationale: 'Obstetric trauma surgeon and neonatology emergency resuscitation team standing by.',
    };
  } else if (patient.hasPoisoning) {
    targetHospital = {
      id: 'HOSP-06',
      name: 'Central Government Toxicology & Poison Control Center',
      specialty: 'Clinical Toxicology, Hemodialysis & Antidote Repository',
      distanceKm: patient.estimatedDistanceKm + 1.4,
      traumaBaysAvailable: 5,
      icuBedsAvailable: 7,
      erWaitMinutes: 2,
      suitabilityScore: 95,
      rationale: 'Antidote repository stocked; continuous toxicologic monitoring protocol activated.',
    };
  }

  // 8. Automatic Actions & Corridor Parameters
  const greenCorridorLength = priorityClass === 'P1_CRITICAL' ? 8 : priorityClass === 'P2_HIGH' ? 6 : priorityClass === 'P3_MEDIUM' ? 4 : 2;
  const maxSignalHoldSec = priorityClass === 'P1_CRITICAL' ? 60 : priorityClass === 'P2_HIGH' ? 45 : priorityClass === 'P3_MEDIUM' ? 30 : 15;
  const maxWaitingSec = priorityClass === 'P1_CRITICAL' ? 0 : priorityClass === 'P2_HIGH' ? 10 : priorityClass === 'P3_MEDIUM' ? 25 : 40;
  const trafficAggressiveness = priorityClass === 'P1_CRITICAL' ? 95 : priorityClass === 'P2_HIGH' ? 75 : priorityClass === 'P3_MEDIUM' ? 50 : 25;

  const policeEscort =
    patient.policeEscortRequired || patient.vipConflict || priorityClass === 'P1_CRITICAL'
      ? 'MANDATORY'
      : priorityClass === 'P2_HIGH'
      ? 'ADVISORY'
      : 'NONE';

  const hospitalAlert =
    priorityClass === 'P1_CRITICAL'
      ? 'CODE_RED_INSTANT'
      : priorityClass === 'P2_HIGH'
      ? 'CODE_YELLOW_PREP'
      : 'CODE_GREEN_NOTIFICATION';

  const doctorSpecialties: string[] = [];
  if (patient.hasCardiacArrest || patient.hasHeartAttack) doctorSpecialties.push('Interventional Cardiologist', 'Cardiothoracic Surgeon');
  if (patient.hasStroke) doctorSpecialties.push('Vascular Neurologist', 'Interventional Radiologist');
  if (patient.hasTrauma || patient.hasAccident || patient.hasMultipleFractures) doctorSpecialties.push('Trauma Surgeon', 'Orthopedic Surgeon');
  if (patient.hasHeadInjury) doctorSpecialties.push('Neurosurgeon');
  if (patient.hasBurn) doctorSpecialties.push('Burn Specialist', 'Plastic Reconstruction Surgeon');
  if (patient.isPregnant) doctorSpecialties.push('Maternal-Fetal Specialist', 'Neonatologist');
  if (patient.hasPoisoning) doctorSpecialties.push('Medical Toxicologist');
  if (patient.age < 14) doctorSpecialties.push('Pediatric Emergency Physician');
  if (doctorSpecialties.length === 0) doctorSpecialties.push('Attending Emergency Physician');

  const requiredResources: string[] = [];
  if (patient.ventilatorRequired) requiredResources.push('Mechanical Ventilator');
  if (patient.oxygenRequired) requiredResources.push('High-Flow O2 Supply');
  if (patient.icuRequired) requiredResources.push('Critical Care ICU Bed');
  if (patient.bloodLoss !== 'None') requiredResources.push('Rapid Blood Infuser & Type O- Units');
  if (policeEscort === 'MANDATORY') requiredResources.push('Police Escort Cruiser');

  const etaTargetMinutes = Math.max(
    1.5,
    Math.round(((patient.estimatedDistanceKm / (priorityClass === 'P1_CRITICAL' ? 65 : 45)) * 60) * 10) / 10
  );

  return {
    priorityScore,
    priorityClass,
    priorityLabel,
    priorityColor,
    severityBreakdown: {
      medicalSeverity,
      trafficSeverity,
      environmentalSeverity,
      routeComplexity,
      hospitalAvailability,
      vehicleReadiness,
      roadCongestion,
      nodeFailures,
      signalAvailability,
      weatherImpact,
    },
    recommendedHospital: targetHospital,
    automaticActions: {
      greenCorridorLength,
      corridorStrategy:
        priorityClass === 'P1_CRITICAL'
          ? 'Continuous Preemptive Green Wave (All-Red Conflict Lockdown)'
          : 'Dynamic Cascading Signal Preemption',
      maxSignalHoldSec,
      maxWaitingSec,
      policeEscort,
      hospitalAlert,
      doctorSpecialtiesAlerted: doctorSpecialties,
      trafficBroadcast: priorityClass === 'P1_CRITICAL' || priorityClass === 'P2_HIGH',
      etaTargetMinutes,
      trafficAggressivenessPercent: trafficAggressiveness,
      signalOverrideLevel:
        priorityClass === 'P1_CRITICAL'
          ? 'LEVEL_4_FULL_AXIS'
          : priorityClass === 'P2_HIGH'
          ? 'LEVEL_3_CASCADE'
          : 'LEVEL_2_LOCAL',
      requiredResources,
    },
  };
}
