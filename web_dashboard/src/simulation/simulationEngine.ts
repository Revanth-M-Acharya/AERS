import {
  IntersectionState,
  RoadData,
  Landmark,
  RouteOption,
  AmbulanceTelemetry,
  HospitalStatus,
  SystemMetrics,
  EventLogEntry,
  WeatherCondition,
} from '../types';
import {
  createInitialIntersections,
  createInitialRoads,
  createInitialLandmarks,
  calculateMultiCriteriaRoutes,
  evaluateRoadIRSA,
} from './cityGraph';

export interface SimulationState {
  isRunning: boolean;
  isPaused: boolean;
  stage: number; // 0=Idle, 1=Dispatched, 2=Int1, 3=Int2, 4=Int3, 5=Arrived
  elapsedSeconds: number;
  intersections: IntersectionState[];
  roads: RoadData[];
  landmarks: Landmark[];
  routes: {
    primaryRoute: RouteOption;
    altRouteA: RouteOption;
    altRouteB: RouteOption;
  };
  activeRouteId: 'PRIMARY' | 'ALT_A' | 'ALT_B';
  ambulance: AmbulanceTelemetry;
  hospital: HospitalStatus;
  metrics: SystemMetrics;
  weather: WeatherCondition;
  events: EventLogEntry[];
  selectedRoadId: string | null;
  selectedNodeId: number | null;
  // Fault Injection Flags
  policeOverride: boolean;
  policeEscort: { active: boolean; unitId?: string };
  roadBlocked: boolean;
  heavyTraffic: boolean;
  accidentOnRoute: boolean;
  intersectionFailed: boolean;
  hospitalBusy: boolean;
  floodActive: boolean;
  constructionActive: boolean;
  bridgeClosed: boolean;
  cameraFailure: boolean;
  // Dynamic Re-routing & Node Failure Alert
  nodeFailureAlert: string | null;
  // Judge Smart Demo
  smartDemoActive: boolean;
  smartDemoStep: number;
  smartDemoNarration: string | null;
  replayHistory: Array<{
    timestamp: number;
    ambulancePos: number;
    intersections: IntersectionState[];
    speed: number;
  }>;
}

export const createInitialSimulationState = (): SimulationState => {
  const intersections = createInitialIntersections();
  const roads = createInitialRoads();
  const landmarks = createInitialLandmarks();
  const weather: WeatherCondition = {
    mode: 'CLEAR',
    visibilityPercent: 100,
    frictionIndex: 0.95,
  };

  // Evaluate initial IRSA for all roads
  const evaluatedRoads = roads.map((r) => evaluateRoadIRSA(r, weather));
  const initialRoutes = calculateMultiCriteriaRoutes(intersections, evaluatedRoads, 1, 3);

  return {
    isRunning: false,
    isPaused: false,
    stage: 0,
    elapsedSeconds: 0,
    intersections,
    roads: evaluatedRoads,
    landmarks,
    routes: initialRoutes,
    activeRouteId: 'PRIMARY',
    selectedRoadId: null,
    selectedNodeId: null,
    policeOverride: false,
    policeEscort: { active: false, unitId: 'PATROL-01' },
    roadBlocked: false,
    heavyTraffic: false,
    accidentOnRoute: false,
    intersectionFailed: false,
    hospitalBusy: false,
    floodActive: false,
    constructionActive: false,
    bridgeClosed: false,
    cameraFailure: false,
    nodeFailureAlert: null,
    smartDemoActive: false,
    smartDemoStep: 0,
    smartDemoNarration: null,
    ambulance: {
      id: 'AMB-101',
      callSign: 'Aegis Guardian 1',
      speedKmh: 0,
      positionPercent: 0,
      currentRoad: 'Station Alpha Depot',
      currentSegmentIndex: 0,
      nextIntersectionId: 1,
      etaSeconds: 0,
      destination: 'City General Hospital (Trauma Center)',
      targetHospitalId: 'HOSP-01',
      patientCondition: 'Stable / Standby',
      heartRateBpm: 76,
      spO2Percent: 98,
      bloodPressure: '120/80',
      priority: 'P1_CRITICAL',
      isActive: false,
      isBreakdown: false,
    },
    hospital: {
      id: 'HOSP-01',
      name: 'City General Hospital',
      traumaBaysAvailable: 4,
      totalTraumaBays: 6,
      orReady: true,
      erWaitMinutes: 4,
      receivingTeamAlerted: false,
      status: 'GREEN',
    },
    metrics: {
      coordinatorHealth: 'ONLINE',
      coordinatorIp: '10.204.179.4',
      clusterUptimeSeconds: 1420,
      totalMissions: 28,
      successfulMissions: 28,
      averageEtaReductionPercent: 61.8,
      packetsPerSec: 148,
      latencyMs: 0.8,
      cpuUsagePercent: 14.2,
      ramUsagePercent: 32.5,
    },
    weather,
    events: [
      {
        id: 'EVT-001',
        timestamp: new Date().toLocaleTimeString(),
        source: 'COORDINATOR',
        level: 'INFO',
        message: 'Central Coordinator initialized on Pi 1 (10.204.179.4)',
      },
      {
        id: 'EVT-002',
        timestamp: new Date().toLocaleTimeString(),
        source: 'SYSTEM',
        level: 'SUCCESS',
        message: 'IRSA Road Situation Awareness Graph online: 12 nodes, 22 roads active.',
      },
    ],
    replayHistory: [],
  };
};
