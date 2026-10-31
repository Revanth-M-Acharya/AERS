export type OperationMode = 'SIMULATION' | 'LIVE';

export type EmergencyPriority =
  | 'P1_CRITICAL'
  | 'P2_HIGH'
  | 'P3_MEDIUM'
  | 'P4_LOW'
  | 'P2_URGENT'
  | 'P3_STANDARD';

export type SignalLightColor = 'RED' | 'YELLOW' | 'GREEN';

export type NodeHealthStatus = 'ONLINE' | 'OFFLINE' | 'RECONNECTING' | 'ERROR';

export type TrafficDensityLevel = 'GREEN' | 'YELLOW' | 'ORANGE' | 'RED';

export type RoadConditionType = 'EXCELLENT' | 'GOOD' | 'WET' | 'DAMAGED' | 'BLOCKED';

export type VisibilityType = 'CLEAR' | 'RAIN' | 'FOG' | 'SMOKE';

export type IncidentType =
  | 'NORMAL'
  | 'ACCIDENT'
  | 'VEHICLE_BREAKDOWN'
  | 'POLICE_ACTIVITY'
  | 'ROAD_CLOSED'
  | 'FLOOD'
  | 'CONSTRUCTION'
  | 'BRIDGE_CLOSED';

export interface CameraPerception {
  id: string;
  name: string;
  health: 'ONLINE' | 'OFFLINE';
  fps: number;
  vehicleCount: number;
  emergencyVehicleDetected: boolean;
  queueLength: number; // in vehicles
  laneOccupancyPercent: number; // 0 to 100%
}

export interface RoadData {
  id: string;
  name: string;
  fromNodeId: number;
  toNodeId: number;
  lengthKm: number;
  speedLimitKmh: number;
  trafficDensity: TrafficDensityLevel;
  congestionPercent: number; // 0 to 100%
  accidentProbability: number; // 0 to 100%
  roadCondition: RoadConditionType;
  constructionStatus: boolean;
  floodStatus: boolean;
  visibility: VisibilityType;
  travelTimeSeconds: number;
  historicalTravelTime: number;
  predictedNext5Min: number;
  weight: number; // Computed cost for routing
  confidenceScore: number; // 0 to 100%
  suitabilityScore: number; // 0 to 100%
  incident: IncidentType;
  camera?: CameraPerception;
  isBlocked?: boolean;
  hasAccident?: boolean;
  isFlooded?: boolean;
  isUnderConstruction?: boolean;
  isBridgeClosed?: boolean;
}

export interface IntersectionState {
  id: number;
  name: string;
  x: number; // Canvas coordinate
  y: number; // Canvas coordinate
  ip: string;
  port: number;
  health: NodeHealthStatus;
  blueLed: boolean;
  redLed: boolean;
  hasRedHardware: boolean; // false for Node 3 (Hardware Shortage)
  signalColor: SignalLightColor;
  countdown: number;
  preempted: boolean;
  irSensorTriggered: boolean;
  oledStatus: string;
  crossTrafficDensity: number; // 0 to 100%
  cpuPercent: number;
  temperatureC: number;
  latencyMs: number;
  packetLossPercent: number;
  trafficQueueLength: number;
  queueLength?: number;
  camera?: CameraPerception;
}

export interface Landmark {
  id: string;
  name: string;
  type: 'HOSPITAL' | 'AMBULANCE_DEPOT' | 'POLICE_PRECINCT' | 'FIRE_STATION' | 'COORDINATOR';
  x: number;
  y: number;
  nearestNodeId: number;
  statusText?: string;
}

export interface RouteOption {
  id: 'PRIMARY' | 'ALT_A' | 'ALT_B';
  name: string;
  nodeIds: number[];
  roadIds: string[];
  totalDistanceKm: number;
  distanceKm?: number;
  etaSeconds: number;
  confidencePercent: number;
  confidenceScore?: number;
  reason: string;
  isBlocked: boolean;
  active: boolean;
}

export interface AmbulanceTelemetry {
  id: string;
  callSign: string;
  speedKmh: number;
  positionPercent: number; // 0 to 100% of route
  currentRoad: string;
  currentSegmentIndex: number;
  nextIntersectionId: number | null;
  etaSeconds: number;
  destination: string;
  targetHospitalId: string;
  patientCondition: string;
  heartRateBpm: number;
  spO2Percent: number;
  bloodPressure: string;
  priority: EmergencyPriority;
  isActive: boolean;
  isBreakdown: boolean;
}

export interface HospitalStatus {
  id: string;
  name: string;
  traumaBaysAvailable: number;
  totalTraumaBays: number;
  orReady: boolean;
  erWaitMinutes: number;
  receivingTeamAlerted: boolean;
  status: 'GREEN' | 'YELLOW' | 'RED';
}

export interface SystemMetrics {
  coordinatorHealth: NodeHealthStatus;
  coordinatorIp: string;
  clusterUptimeSeconds: number;
  totalMissions: number;
  successfulMissions: number;
  averageEtaReductionPercent: number;
  packetsPerSec: number;
  latencyMs: number;
  cpuUsagePercent: number;
  ramUsagePercent: number;
}

export interface EventLogEntry {
  id: string;
  timestamp: string;
  source: string;
  level: 'INFO' | 'WARN' | 'ERROR' | 'EMERGENCY' | 'SUCCESS';
  message: string;
  details?: string;
}

export interface EmergencyCall {
  id: string;
  caller: string;
  location: string;
  timestamp: string;
  nature: string;
  priority: EmergencyPriority;
  status: 'QUEUED' | 'DISPATCHED' | 'EN_ROUTE' | 'RESOLVED';
  assignedAmbulance?: string;
}

export interface WeatherCondition {
  mode: 'CLEAR' | 'NIGHT' | 'RAIN' | 'FOG';
  visibilityPercent: number;
  frictionIndex: number;
}

export interface SmartDemoStep {
  step: number;
  title: string;
  description: string;
  durationMs: number;
}

// =============================================================================
// Feature 1: Intelligent Priority Scheduler & IRSA Engine Types
// =============================================================================

export type PriorityGrade = 'P1_CRITICAL' | 'P2_HIGH' | 'P3_MEDIUM' | 'P4_LOW';

export interface PatientCase {
  patientName: string;
  age: number;
  gender: 'Male' | 'Female' | 'Other';
  weightKg: number;
  heartRate: number; // bpm
  bloodPressureSystolic: number;
  bloodPressureDiastolic: number;
  spO2: number; // %
  respirationRate: number; // breaths/min
  temperatureC: number; // Celsius
  gcsScore: number; // Glasgow Coma Scale 3-15
  painLevel: number; // 1-10
  bloodLoss: 'None' | 'Mild' | 'Moderate' | 'Severe' | 'Massive';
  // Conditions
  isPregnant: boolean;
  hasTrauma: boolean;
  hasBurn: boolean;
  hasStroke: boolean;
  hasHeartAttack: boolean;
  hasCardiacArrest: boolean;
  hasPoisoning: boolean;
  hasAccident: boolean;
  hasHeadInjury: boolean;
  hasMultipleFractures: boolean;
  hasInternalBleeding: boolean;
  isDiabetic: boolean;
  hasAsthma: boolean;
  hasSeizure: boolean;
  hasCancer: boolean;
  // Resource Needs
  ventilatorRequired: boolean;
  oxygenRequired: boolean;
  icuRequired: boolean;
  // Contextual & Environment
  estimatedDistanceKm: number;
  trafficDensity: 'Low' | 'Moderate' | 'Heavy' | 'Gridlock';
  weather: 'Clear' | 'Rain' | 'Fog' | 'Storm';
  manualOverride: boolean;
  policeEscortRequired: boolean;
  vipConflict: boolean;
}

export interface IRSAResult {
  priorityScore: number; // 0 - 100
  priorityClass: PriorityGrade;
  priorityLabel: 'P1 Critical' | 'P2 High' | 'P3 Medium' | 'P4 Low';
  priorityColor: string; // Hex color code
  severityBreakdown: {
    medicalSeverity: number; // 0 - 100
    trafficSeverity: number; // 0 - 100
    environmentalSeverity: number; // 0 - 100
    routeComplexity: number; // 0 - 100
    hospitalAvailability: number; // 0 - 100
    vehicleReadiness: number; // 0 - 100
    roadCongestion: number; // 0 - 100
    nodeFailures: number; // 0 - 100
    signalAvailability: number; // 0 - 100
    weatherImpact: number; // 0 - 100
  };
  recommendedHospital: {
    id: string;
    name: string;
    specialty: string;
    distanceKm: number;
    traumaBaysAvailable: number;
    icuBedsAvailable: number;
    erWaitMinutes: number;
    suitabilityScore: number; // 0 - 100
    rationale: string;
  };
  automaticActions: {
    greenCorridorLength: number; // Number of intersections
    corridorStrategy: string;
    maxSignalHoldSec: number;
    maxWaitingSec: number;
    policeEscort: 'MANDATORY' | 'ADVISORY' | 'STANDBY' | 'NONE';
    hospitalAlert: 'CODE_RED_INSTANT' | 'CODE_YELLOW_PREP' | 'CODE_GREEN_NOTIFICATION' | 'ROUTINE';
    doctorSpecialtiesAlerted: string[];
    trafficBroadcast: boolean;
    etaTargetMinutes: number;
    trafficAggressivenessPercent: number;
    signalOverrideLevel: 'LEVEL_4_FULL_AXIS' | 'LEVEL_3_CASCADE' | 'LEVEL_2_LOCAL' | 'LEVEL_1_ROUTINE';
    requiredResources: string[];
  };
}

// =============================================================================
// Feature 2: Ambulance Driver Dashboard Types
// =============================================================================

export type CameraFeedId = 'FRONT' | 'REAR' | 'CABIN' | 'PATIENT';

export interface DriverAlert {
  id: string;
  type:
    | 'ROAD_CLOSED'
    | 'HOSPITAL_BUSY'
    | 'NODE_FAILURE'
    | 'TRAFFIC_CONGESTION'
    | 'POLICE_ESCORT'
    | 'ALTERNATIVE_ROUTE'
    | 'COMMUNICATION_LOST';
  title: string;
  message: string;
  severity: 'CRITICAL' | 'WARNING' | 'INFO';
  timestamp: string;
  active: boolean;
}

