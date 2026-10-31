import {
  IntersectionState,
  RoadData,
  Landmark,
  RouteOption,
  WeatherCondition,
  IncidentType,
  CameraPerception,
} from '../types';

export const createInitialIntersections = (): IntersectionState[] => [
  {
    id: 1,
    name: 'MG Road - Brigade Junction',
    x: 200,
    y: 280,
    ip: '10.204.179.245',
    port: 5001,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'GREEN',
    countdown: 18,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'NORMAL CYCLE',
    crossTrafficDensity: 42,
    cpuPercent: 12,
    temperatureC: 43.5,
    latencyMs: 0.8,
    packetLossPercent: 0.0,
    trafficQueueLength: 4,
    camera: {
      id: 'CAM-01',
      name: 'MG Rd 360-PTZ',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 14,
      emergencyVehicleDetected: false,
      queueLength: 4,
      laneOccupancyPercent: 38,
    },
  },
  {
    id: 2,
    name: 'Residency Road Cross',
    x: 450,
    y: 280,
    ip: '10.204.179.130',
    port: 5002,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'RED',
    countdown: 12,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'NORMAL CYCLE',
    crossTrafficDensity: 58,
    cpuPercent: 14,
    temperatureC: 44.1,
    latencyMs: 0.9,
    packetLossPercent: 0.0,
    trafficQueueLength: 7,
    camera: {
      id: 'CAM-02',
      name: 'Residency North-Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 22,
      emergencyVehicleDetected: false,
      queueLength: 7,
      laneOccupancyPercent: 62,
    },
  },
  {
    id: 3,
    name: 'Richmond Circle',
    x: 720,
    y: 280,
    ip: '10.204.179.144',
    port: 5003,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: false, // Node 3 Hardware Shortage
    signalColor: 'GREEN',
    countdown: 25,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'NORMAL (SHORTAGE)',
    crossTrafficDensity: 35,
    cpuPercent: 11,
    temperatureC: 42.8,
    latencyMs: 0.7,
    packetLossPercent: 0.0,
    trafficQueueLength: 3,
    camera: {
      id: 'CAM-03',
      name: 'Richmond Flyover 4K',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 11,
      emergencyVehicleDetected: false,
      queueLength: 3,
      laneOccupancyPercent: 32,
    },
  },
  {
    id: 4,
    name: 'Trinity Circle',
    x: 960,
    y: 280,
    ip: '10.204.179.150',
    port: 5004,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'YELLOW',
    countdown: 5,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 4',
    crossTrafficDensity: 45,
    cpuPercent: 9,
    temperatureC: 41.2,
    latencyMs: 1.1,
    packetLossPercent: 0.0,
    trafficQueueLength: 5,
    camera: {
      id: 'CAM-04',
      name: 'Trinity East AI-Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 16,
      emergencyVehicleDetected: false,
      queueLength: 5,
      laneOccupancyPercent: 44,
    },
  },
  {
    id: 5,
    name: 'Victoria Hospital Junction',
    x: 200,
    y: 480,
    ip: '10.204.179.151',
    port: 5005,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'RED',
    countdown: 14,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 5',
    crossTrafficDensity: 30,
    cpuPercent: 8,
    temperatureC: 40.5,
    latencyMs: 1.0,
    packetLossPercent: 0.0,
    trafficQueueLength: 2,
    camera: {
      id: 'CAM-05',
      name: 'Victoria South Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 8,
      emergencyVehicleDetected: false,
      queueLength: 2,
      laneOccupancyPercent: 24,
    },
  },
  {
    id: 6,
    name: 'Koramangala Ring Road',
    x: 450,
    y: 480,
    ip: '10.204.179.152',
    port: 5006,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'GREEN',
    countdown: 22,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 6',
    crossTrafficDensity: 65,
    cpuPercent: 15,
    temperatureC: 45.0,
    latencyMs: 1.2,
    packetLossPercent: 0.0,
    trafficQueueLength: 8,
    camera: {
      id: 'CAM-06',
      name: 'Koramangala 80ft Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 26,
      emergencyVehicleDetected: false,
      queueLength: 8,
      laneOccupancyPercent: 68,
    },
  },
  {
    id: 7,
    name: 'Indiranagar 100ft Junction',
    x: 720,
    y: 480,
    ip: '10.204.179.153',
    port: 5007,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'RED',
    countdown: 16,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 7',
    crossTrafficDensity: 48,
    cpuPercent: 10,
    temperatureC: 42.1,
    latencyMs: 0.9,
    packetLossPercent: 0.0,
    trafficQueueLength: 4,
    camera: {
      id: 'CAM-07',
      name: 'Indiranagar Metro Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 15,
      emergencyVehicleDetected: false,
      queueLength: 4,
      laneOccupancyPercent: 42,
    },
  },
  {
    id: 8,
    name: 'Old Airport Expressway',
    x: 960,
    y: 480,
    ip: '10.204.179.154',
    port: 5008,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'GREEN',
    countdown: 30,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 8',
    crossTrafficDensity: 38,
    cpuPercent: 11,
    temperatureC: 41.8,
    latencyMs: 1.3,
    packetLossPercent: 0.0,
    trafficQueueLength: 3,
    camera: {
      id: 'CAM-08',
      name: 'Expressway East Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 18,
      emergencyVehicleDetected: false,
      queueLength: 3,
      laneOccupancyPercent: 35,
    },
  },
  {
    id: 9,
    name: 'Cubbon Park North Cross',
    x: 200,
    y: 110,
    ip: '10.204.179.155',
    port: 5009,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'GREEN',
    countdown: 20,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 9',
    crossTrafficDensity: 28,
    cpuPercent: 8,
    temperatureC: 39.5,
    latencyMs: 0.9,
    packetLossPercent: 0.0,
    trafficQueueLength: 2,
    camera: {
      id: 'CAM-09',
      name: 'Cubbon Green Way Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 7,
      emergencyVehicleDetected: false,
      queueLength: 2,
      laneOccupancyPercent: 20,
    },
  },
  {
    id: 10,
    name: 'Queens Road Interlink',
    x: 450,
    y: 110,
    ip: '10.204.179.156',
    port: 5010,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'RED',
    countdown: 15,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 10',
    crossTrafficDensity: 40,
    cpuPercent: 10,
    temperatureC: 41.0,
    latencyMs: 1.0,
    packetLossPercent: 0.0,
    trafficQueueLength: 4,
    camera: {
      id: 'CAM-10',
      name: 'Queens Corridor Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 14,
      emergencyVehicleDetected: false,
      queueLength: 4,
      laneOccupancyPercent: 36,
    },
  },
  {
    id: 11,
    name: 'Cantonment Central',
    x: 720,
    y: 110,
    ip: '10.204.179.157',
    port: 5011,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'GREEN',
    countdown: 24,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 11',
    crossTrafficDensity: 32,
    cpuPercent: 9,
    temperatureC: 40.8,
    latencyMs: 0.8,
    packetLossPercent: 0.0,
    trafficQueueLength: 3,
    camera: {
      id: 'CAM-11',
      name: 'Cantonment Rail Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 10,
      emergencyVehicleDetected: false,
      queueLength: 3,
      laneOccupancyPercent: 28,
    },
  },
  {
    id: 12,
    name: 'Halasuru Lake Overpass',
    x: 960,
    y: 110,
    ip: '10.204.179.158',
    port: 5012,
    health: 'ONLINE',
    blueLed: false,
    redLed: false,
    hasRedHardware: true,
    signalColor: 'YELLOW',
    countdown: 4,
    preempted: false,
    irSensorTriggered: false,
    oledStatus: 'EDGE NODE 12',
    crossTrafficDensity: 50,
    cpuPercent: 12,
    temperatureC: 43.0,
    latencyMs: 1.1,
    packetLossPercent: 0.0,
    trafficQueueLength: 5,
    camera: {
      id: 'CAM-12',
      name: 'Halasuru Bridge Cam',
      health: 'ONLINE',
      fps: 30,
      vehicleCount: 19,
      emergencyVehicleDetected: false,
      queueLength: 5,
      laneOccupancyPercent: 48,
    },
  },
];

export const createInitialRoads = (): RoadData[] => [
  // Horizontal Arterials - Main West-East Corridor
  {
    id: 'ROAD-01',
    name: 'MG Road Expressway',
    fromNodeId: 1,
    toNodeId: 2,
    lengthKm: 1.4,
    speedLimitKmh: 60,
    trafficDensity: 'GREEN',
    congestionPercent: 28,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 84,
    historicalTravelTime: 80,
    predictedNext5Min: 85,
    weight: 1.4,
    confidenceScore: 94,
    suitabilityScore: 95,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-02',
    name: 'Residency Express Link',
    fromNodeId: 2,
    toNodeId: 3,
    lengthKm: 1.6,
    speedLimitKmh: 60,
    trafficDensity: 'GREEN',
    congestionPercent: 34,
    accidentProbability: 6,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 96,
    historicalTravelTime: 92,
    predictedNext5Min: 98,
    weight: 1.6,
    confidenceScore: 91,
    suitabilityScore: 90,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-03',
    name: 'Richmond Flyover',
    fromNodeId: 3,
    toNodeId: 4,
    lengthKm: 1.5,
    speedLimitKmh: 65,
    trafficDensity: 'GREEN',
    congestionPercent: 25,
    accidentProbability: 3,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 83,
    historicalTravelTime: 80,
    predictedNext5Min: 84,
    weight: 1.5,
    confidenceScore: 96,
    suitabilityScore: 94,
    incident: 'NORMAL',
  },

  // Horizontal South Corridor
  {
    id: 'ROAD-04',
    name: 'Victoria South Boulevard',
    fromNodeId: 5,
    toNodeId: 6,
    lengthKm: 1.5,
    speedLimitKmh: 50,
    trafficDensity: 'YELLOW',
    congestionPercent: 48,
    accidentProbability: 8,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 108,
    historicalTravelTime: 100,
    predictedNext5Min: 115,
    weight: 2.2,
    confidenceScore: 88,
    suitabilityScore: 82,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-05',
    name: 'Koramangala Ringway',
    fromNodeId: 6,
    toNodeId: 7,
    lengthKm: 1.6,
    speedLimitKmh: 55,
    trafficDensity: 'ORANGE',
    congestionPercent: 68,
    accidentProbability: 14,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 140,
    historicalTravelTime: 110,
    predictedNext5Min: 155,
    weight: 3.4,
    confidenceScore: 78,
    suitabilityScore: 65,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-06',
    name: 'Old Airport Link',
    fromNodeId: 7,
    toNodeId: 8,
    lengthKm: 1.4,
    speedLimitKmh: 60,
    trafficDensity: 'GREEN',
    congestionPercent: 30,
    accidentProbability: 5,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 84,
    historicalTravelTime: 82,
    predictedNext5Min: 86,
    weight: 1.5,
    confidenceScore: 93,
    suitabilityScore: 92,
    incident: 'NORMAL',
  },

  // Horizontal North Corridor
  {
    id: 'ROAD-07',
    name: 'Cubbon North Way',
    fromNodeId: 9,
    toNodeId: 10,
    lengthKm: 1.4,
    speedLimitKmh: 50,
    trafficDensity: 'GREEN',
    congestionPercent: 22,
    accidentProbability: 3,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 100,
    historicalTravelTime: 98,
    predictedNext5Min: 102,
    weight: 1.5,
    confidenceScore: 95,
    suitabilityScore: 93,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-08',
    name: 'Queens Interlink',
    fromNodeId: 10,
    toNodeId: 11,
    lengthKm: 1.6,
    speedLimitKmh: 55,
    trafficDensity: 'YELLOW',
    congestionPercent: 42,
    accidentProbability: 6,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 105,
    historicalTravelTime: 100,
    predictedNext5Min: 110,
    weight: 2.1,
    confidenceScore: 89,
    suitabilityScore: 85,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-09',
    name: 'Cantonment Lake Avenue',
    fromNodeId: 11,
    toNodeId: 12,
    lengthKm: 1.5,
    speedLimitKmh: 55,
    trafficDensity: 'GREEN',
    congestionPercent: 28,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 98,
    historicalTravelTime: 95,
    predictedNext5Min: 100,
    weight: 1.6,
    confidenceScore: 94,
    suitabilityScore: 92,
    incident: 'NORMAL',
  },

  // Vertical Interconnectors
  {
    id: 'ROAD-10',
    name: 'Brigade North Cross',
    fromNodeId: 9,
    toNodeId: 1,
    lengthKm: 1.1,
    speedLimitKmh: 45,
    trafficDensity: 'GREEN',
    congestionPercent: 20,
    accidentProbability: 3,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 88,
    historicalTravelTime: 85,
    predictedNext5Min: 90,
    weight: 1.2,
    confidenceScore: 96,
    suitabilityScore: 94,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-11',
    name: 'Brigade South Way',
    fromNodeId: 1,
    toNodeId: 5,
    lengthKm: 1.2,
    speedLimitKmh: 45,
    trafficDensity: 'GREEN',
    congestionPercent: 24,
    accidentProbability: 4,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 96,
    historicalTravelTime: 92,
    predictedNext5Min: 98,
    weight: 1.3,
    confidenceScore: 93,
    suitabilityScore: 91,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-12',
    name: 'Queens Residency Link',
    fromNodeId: 10,
    toNodeId: 2,
    lengthKm: 1.1,
    speedLimitKmh: 50,
    trafficDensity: 'YELLOW',
    congestionPercent: 44,
    accidentProbability: 7,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 90,
    historicalTravelTime: 80,
    predictedNext5Min: 95,
    weight: 1.8,
    confidenceScore: 87,
    suitabilityScore: 84,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-13',
    name: 'Residency Koramangala Axis',
    fromNodeId: 2,
    toNodeId: 6,
    lengthKm: 1.2,
    speedLimitKmh: 50,
    trafficDensity: 'ORANGE',
    congestionPercent: 62,
    accidentProbability: 12,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 120,
    historicalTravelTime: 95,
    predictedNext5Min: 135,
    weight: 2.8,
    confidenceScore: 80,
    suitabilityScore: 70,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-14',
    name: 'Cantonment Richmond Cut',
    fromNodeId: 11,
    toNodeId: 3,
    lengthKm: 1.1,
    speedLimitKmh: 50,
    trafficDensity: 'GREEN',
    congestionPercent: 26,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 80,
    historicalTravelTime: 78,
    predictedNext5Min: 82,
    weight: 1.2,
    confidenceScore: 95,
    suitabilityScore: 94,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-15',
    name: 'Richmond Indiranagar Connector',
    fromNodeId: 3,
    toNodeId: 7,
    lengthKm: 1.2,
    speedLimitKmh: 50,
    trafficDensity: 'GREEN',
    congestionPercent: 32,
    accidentProbability: 5,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 86,
    historicalTravelTime: 84,
    predictedNext5Min: 88,
    weight: 1.3,
    confidenceScore: 92,
    suitabilityScore: 91,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-16',
    name: 'Halasuru Trinity Link',
    fromNodeId: 12,
    toNodeId: 4,
    lengthKm: 1.1,
    speedLimitKmh: 50,
    trafficDensity: 'YELLOW',
    congestionPercent: 40,
    accidentProbability: 5,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 88,
    historicalTravelTime: 82,
    predictedNext5Min: 92,
    weight: 1.7,
    confidenceScore: 90,
    suitabilityScore: 86,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-17',
    name: 'Trinity Airport Connector',
    fromNodeId: 4,
    toNodeId: 8,
    lengthKm: 1.2,
    speedLimitKmh: 55,
    trafficDensity: 'GREEN',
    congestionPercent: 30,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 80,
    historicalTravelTime: 78,
    predictedNext5Min: 82,
    weight: 1.3,
    confidenceScore: 94,
    suitabilityScore: 93,
    incident: 'NORMAL',
  },

  // Diagonal / High-Speed Bypasses
  {
    id: 'ROAD-18',
    name: 'Queens to MG Diagonal Bypass',
    fromNodeId: 10,
    toNodeId: 1,
    lengthKm: 1.8,
    speedLimitKmh: 60,
    trafficDensity: 'GREEN',
    congestionPercent: 22,
    accidentProbability: 3,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 108,
    historicalTravelTime: 105,
    predictedNext5Min: 110,
    weight: 1.9,
    confidenceScore: 95,
    suitabilityScore: 92,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-19',
    name: 'Richmond to Old Airport Bypass',
    fromNodeId: 3,
    toNodeId: 8,
    lengthKm: 2.1,
    speedLimitKmh: 65,
    trafficDensity: 'GREEN',
    congestionPercent: 26,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 116,
    historicalTravelTime: 112,
    predictedNext5Min: 120,
    weight: 2.2,
    confidenceScore: 93,
    suitabilityScore: 91,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-20',
    name: 'Koramangala to Indiranagar Diagonal',
    fromNodeId: 6,
    toNodeId: 3,
    lengthKm: 1.9,
    speedLimitKmh: 55,
    trafficDensity: 'YELLOW',
    congestionPercent: 46,
    accidentProbability: 8,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 124,
    historicalTravelTime: 115,
    predictedNext5Min: 130,
    weight: 2.6,
    confidenceScore: 86,
    suitabilityScore: 80,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-21',
    name: 'Halasuru Overpass Bridge',
    fromNodeId: 11,
    toNodeId: 4,
    lengthKm: 1.8,
    speedLimitKmh: 60,
    trafficDensity: 'GREEN',
    congestionPercent: 28,
    accidentProbability: 4,
    roadCondition: 'EXCELLENT',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 108,
    historicalTravelTime: 104,
    predictedNext5Min: 112,
    weight: 1.9,
    confidenceScore: 94,
    suitabilityScore: 92,
    incident: 'NORMAL',
  },
  {
    id: 'ROAD-22',
    name: 'Victoria Koramangala Outer Link',
    fromNodeId: 5,
    toNodeId: 2,
    lengthKm: 2.0,
    speedLimitKmh: 50,
    trafficDensity: 'GREEN',
    congestionPercent: 32,
    accidentProbability: 5,
    roadCondition: 'GOOD',
    constructionStatus: false,
    floodStatus: false,
    visibility: 'CLEAR',
    travelTimeSeconds: 144,
    historicalTravelTime: 140,
    predictedNext5Min: 148,
    weight: 2.3,
    confidenceScore: 91,
    suitabilityScore: 89,
    incident: 'NORMAL',
  },
];

export const createInitialLandmarks = (): Landmark[] => [
  {
    id: 'HOSP-01',
    name: 'City General Hospital (Trauma Center)',
    type: 'HOSPITAL',
    x: 720,
    y: 350,
    nearestNodeId: 3,
    statusText: 'Trauma Bay 4 Ready',
  },
  {
    id: 'HOSP-02',
    name: 'Apollo Specialty Hospital',
    type: 'HOSPITAL',
    x: 1040,
    y: 480,
    nearestNodeId: 8,
    statusText: 'Cardiology Ready',
  },
  {
    id: 'HOSP-03',
    name: "St. Martha's Hospital",
    type: 'HOSPITAL',
    x: 320,
    y: 50,
    nearestNodeId: 9,
    statusText: 'ER Standby',
  },
  {
    id: 'STN-ALPHA',
    name: 'Ambulance Station Alpha (West)',
    type: 'AMBULANCE_DEPOT',
    x: 90,
    y: 280,
    nearestNodeId: 1,
    statusText: 'AMB-101 Ready',
  },
  {
    id: 'STN-BETA',
    name: 'Ambulance Station Beta (East)',
    type: 'AMBULANCE_DEPOT',
    x: 1040,
    y: 110,
    nearestNodeId: 12,
    statusText: 'AMB-102 Ready',
  },
  {
    id: 'POLICE-01',
    name: 'Central Police Precinct',
    type: 'POLICE_PRECINCT',
    x: 450,
    y: 190,
    nearestNodeId: 2,
    statusText: 'Patrol Active',
  },
  {
    id: 'POLICE-02',
    name: 'North Police Precinct',
    type: 'POLICE_PRECINCT',
    x: 840,
    y: 110,
    nearestNodeId: 11,
    statusText: 'Corridor Escort Ready',
  },
  {
    id: 'FIRE-01',
    name: 'Metro Fire Station 4',
    type: 'FIRE_STATION',
    x: 450,
    y: 560,
    nearestNodeId: 6,
    statusText: 'Hazmat Available',
  },
  {
    id: 'COORD-01',
    name: 'Central Master Coordinator',
    type: 'COORDINATOR',
    x: 580,
    y: 40,
    nearestNodeId: 10,
    statusText: 'Pi 1: 10.204.179.4 (Master)',
  },
];

/**
 * Intelligent Road Situation Awareness (IRSA) Engine
 * Continuously evaluates road suitability and composite traversal cost.
 */
export const evaluateRoadIRSA = (
  road: RoadData,
  weather: WeatherCondition,
  node1Health: string = 'ONLINE',
  node2Health: string = 'ONLINE'
): RoadData => {
  // Base travel time
  let travelTime = (road.lengthKm / (road.speedLimitKmh || 50)) * 3600;

  // Congestion multiplier
  let congestionMultiplier = 1.0;
  let trafficDensity = road.trafficDensity;

  if (road.congestionPercent > 75) {
    trafficDensity = 'RED';
    congestionMultiplier = 2.4;
  } else if (road.congestionPercent > 50) {
    trafficDensity = 'ORANGE';
    congestionMultiplier = 1.7;
  } else if (road.congestionPercent > 25) {
    trafficDensity = 'YELLOW';
    congestionMultiplier = 1.3;
  } else {
    trafficDensity = 'GREEN';
    congestionMultiplier = 1.0;
  }

  // Condition penalty
  let conditionMultiplier = 1.0;
  let isBlocked = false;

  if (road.roadCondition === 'BLOCKED' || road.incident === 'ROAD_CLOSED' || road.incident === 'BRIDGE_CLOSED') {
    isBlocked = true;
    conditionMultiplier = 1000.0; // Infinite penalty
  } else if (road.roadCondition === 'DAMAGED' || road.incident === 'ACCIDENT') {
    conditionMultiplier = 2.8;
  } else if (road.roadCondition === 'WET' || road.floodStatus) {
    conditionMultiplier = 1.8;
  } else if (road.constructionStatus) {
    conditionMultiplier = 1.6;
  }

  // Weather penalty
  let weatherMultiplier = 1.0;
  if (weather.mode === 'RAIN') weatherMultiplier = 1.25;
  if (weather.mode === 'FOG') weatherMultiplier = 1.45;
  if (weather.mode === 'NIGHT') weatherMultiplier = 1.1;

  // Node health penalty (if an endpoint node is OFFLINE, avoid this road)
  let nodePenalty = 1.0;
  if (node1Health !== 'ONLINE' || node2Health !== 'ONLINE') {
    nodePenalty = 50.0; // Heavy penalty to force routing around dead node
  }

  // Camera confidence factor
  let confidenceScore = 95;
  if (road.camera?.health === 'OFFLINE') {
    confidenceScore -= 20;
  }
  if (road.visibility !== 'CLEAR') {
    confidenceScore -= 15;
  }
  if (road.accidentProbability > 10) {
    confidenceScore -= 10;
  }
  confidenceScore = Math.max(10, Math.min(100, confidenceScore));

  // Compute final effective travel time
  const effectiveTravelTime = Math.round(travelTime * congestionMultiplier * conditionMultiplier * weatherMultiplier);
  const predictedNext5Min = Math.round(effectiveTravelTime * 1.05);

  // Composite routing weight
  const weight = isBlocked
    ? 99999
    : road.lengthKm * congestionMultiplier * conditionMultiplier * weatherMultiplier * nodePenalty;

  // Suitability Score (0-100)
  let suitabilityScore = Math.round(
    Math.max(0, 100 - road.congestionPercent * 0.5 - (isBlocked ? 90 : 0) - (conditionMultiplier > 1.5 ? 25 : 0))
  );

  return {
    ...road,
    trafficDensity,
    travelTimeSeconds: effectiveTravelTime,
    predictedNext5Min,
    weight,
    confidenceScore,
    suitabilityScore,
  };
};

/**
 * Multi-Criteria Route Optimization Engine
 * Generates Primary Route, Alternative Route A, and Alternative Route B.
 */
export const calculateMultiCriteriaRoutes = (
  nodes: IntersectionState[],
  roads: RoadData[],
  startNodeId: number = 1,
  targetNodeId: number = 3
): {
  primaryRoute: RouteOption;
  altRouteA: RouteOption;
  altRouteB: RouteOption;
} => {
  // Helper to build adjacency map
  const adj = new Map<number, Array<{ to: number; road: RoadData }>>();
  nodes.forEach((n) => adj.set(n.id, []));

  roads.forEach((r) => {
    adj.get(r.fromNodeId)?.push({ to: r.toNodeId, road: r });
    adj.get(r.toNodeId)?.push({ to: r.fromNodeId, road: r });
  });

  // Dijkstra implementation using dynamic road weights
  const solvePath = (bannedNodeId: number | null = null, bannedRoadId: string | null = null) => {
    const dist = new Map<number, number>();
    const prevNode = new Map<number, number>();
    const prevRoad = new Map<number, RoadData>();
    const visited = new Set<number>();

    nodes.forEach((n) => dist.set(n.id, Infinity));
    dist.set(startNodeId, 0);

    for (let i = 0; i < nodes.length; i++) {
      let u = -1;
      let minDist = Infinity;
      nodes.forEach((n) => {
        if (!visited.has(n.id) && (dist.get(n.id) ?? Infinity) < minDist) {
          minDist = dist.get(n.id)!;
          u = n.id;
        }
      });

      if (u === -1 || u === targetNodeId) break;
      visited.add(u);

      const neighbors = adj.get(u) || [];
      for (const edge of neighbors) {
        if (edge.to === bannedNodeId || edge.road.id === bannedRoadId) continue;
        if (edge.road.weight >= 99990) continue; // blocked road

        const alt = dist.get(u)! + edge.road.weight;
        if (alt < (dist.get(edge.to) ?? Infinity)) {
          dist.set(edge.to, alt);
          prevNode.set(edge.to, u);
          prevRoad.set(edge.to, edge.road);
        }
      }
    }

    // Reconstruct path
    const pathNodes: number[] = [];
    const pathRoads: string[] = [];
    let curr: number | undefined = targetNodeId;

    if (dist.get(targetNodeId) === Infinity) {
      return null;
    }

    while (curr !== undefined) {
      pathNodes.unshift(curr);
      const r = prevRoad.get(curr);
      if (r) pathRoads.unshift(r.id);
      curr = prevNode.get(curr);
    }

    // Calculate total distance & ETA
    let totalKm = 0;
    let totalEtaSec = 0;
    let confidenceSum = 0;

    pathRoads.forEach((rId) => {
      const rd = roads.find((r) => r.id === rId);
      if (rd) {
        totalKm += rd.lengthKm;
        totalEtaSec += rd.travelTimeSeconds;
        confidenceSum += rd.confidenceScore;
      }
    });

    const avgConfidence = pathRoads.length > 0 ? Math.round(confidenceSum / pathRoads.length) : 85;

    return {
      nodeIds: pathNodes,
      roadIds: pathRoads,
      totalDistanceKm: Math.round(totalKm * 10) / 10,
      etaSeconds: totalEtaSec,
      confidencePercent: avgConfidence,
    };
  };

  // 1. Primary Route: Unconstrained optimal path
  const primaryResult = solvePath();
  const primary: RouteOption = {
    id: 'PRIMARY',
    name: 'Primary Green Corridor',
    nodeIds: primaryResult?.nodeIds ?? [1, 2, 3],
    roadIds: primaryResult?.roadIds ?? ['ROAD-01', 'ROAD-02'],
    totalDistanceKm: primaryResult?.totalDistanceKm ?? 3.0,
    etaSeconds: primaryResult?.etaSeconds ?? 36,
    confidencePercent: primaryResult?.confidencePercent ?? 92,
    reason: 'Direct arterial highway; highest throughput & full preemption active.',
    isBlocked: !primaryResult,
    active: true,
  };

  // 2. Alternative Route A: Avoids Node 2 or mid-corridor road
  const midNode = primary.nodeIds.length > 2 ? primary.nodeIds[1] : 2;
  const altAResult = solvePath(midNode) || solvePath(null, primary.roadIds[0]);
  const altA: RouteOption = {
    id: 'ALT_A',
    name: 'Alternative Corridor A (North Bypass)',
    nodeIds: altAResult?.nodeIds ?? [1, 10, 11, 3],
    roadIds: altAResult?.roadIds ?? ['ROAD-18', 'ROAD-08', 'ROAD-14'],
    totalDistanceKm: altAResult?.totalDistanceKm ?? 4.2,
    etaSeconds: altAResult?.etaSeconds ?? 48,
    confidencePercent: altAResult?.confidencePercent ?? 88,
    reason: 'North Queens Road expressway bypass avoiding central MG-Residency corridor.',
    isBlocked: !altAResult,
    active: false,
  };

  // 3. Alternative Route B: Outer Ring Expressway
  const altBResult = solvePath(null, 'ROAD-02') || solvePath(5);
  const altB: RouteOption = {
    id: 'ALT_B',
    name: 'Alternative Corridor B (Outer Ring Link)',
    nodeIds: altBResult?.nodeIds ?? [1, 5, 6, 7, 3],
    roadIds: altBResult?.roadIds ?? ['ROAD-11', 'ROAD-04', 'ROAD-05', 'ROAD-15'],
    totalDistanceKm: altBResult?.totalDistanceKm ?? 5.5,
    etaSeconds: altBResult?.etaSeconds ?? 62,
    confidencePercent: altBResult?.confidencePercent ?? 82,
    reason: 'South Victoria ringway link; 100% segregated flyovers with zero cross-traffic.',
    isBlocked: !altBResult,
    active: false,
  };

  return { primaryRoute: primary, altRouteA: altA, altRouteB: altB };
};
