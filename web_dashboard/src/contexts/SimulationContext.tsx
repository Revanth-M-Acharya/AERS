import React, { createContext, useContext, useState, useEffect, useRef, useCallback } from 'react';
import {
  OperationMode,
  EventLogEntry,
  EmergencyPriority,
  WeatherCondition,
  RoadData,
  IntersectionState,
  RouteOption,
} from '../types';
import {
  SimulationState,
  createInitialSimulationState,
} from '../simulation/simulationEngine';
import {
  evaluateRoadIRSA,
  calculateMultiCriteriaRoutes,
} from '../simulation/cityGraph';
import { wsService, WebSocketStatus } from '../services/websocketService';

interface SimulationContextValue {
  mode: OperationMode;
  setMode: (mode: OperationMode) => void;
  state: SimulationState;
  wsStatus: WebSocketStatus;
  startMission: (priority?: EmergencyPriority) => void;
  stopMission: () => void;
  pauseSimulation: () => void;
  resumeSimulation: () => void;
  resetSimulation: () => void;
  // Fault Injections
  togglePoliceOverride: () => void;
  toggleRoadBlocked: (roadId?: string) => void;
  toggleHeavyTraffic: () => void;
  toggleAccident: (roadId?: string) => void;
  toggleFlood: (roadId?: string) => void;
  toggleConstruction: (roadId?: string) => void;
  toggleBridgeClosed: (roadId?: string) => void;
  toggleCameraFailure: (camId?: string) => void;
  toggleNodeFailure: (nodeId?: number) => void;
  toggleIntersectionFailure: (nodeId?: number) => void;
  toggleAmbulanceBreakdown: () => void;
  toggleHospitalBusy: () => void;
  // Environment
  setWeather: (weather: WeatherCondition['mode']) => void;
  // Navigation & Selection
  selectRoad: (roadId: string | null) => void;
  selectNode: (nodeId: number | null) => void;
  setActiveRoute: (routeId: 'PRIMARY' | 'ALT_A' | 'ALT_B') => void;
  selectRoute: (routeId: 'PRIMARY' | 'ALT_A' | 'ALT_B') => void;
  // Judge Smart Demo
  startSmartDemo: () => void;
  stopSmartDemo: () => void;
  logEvent: (entry: Omit<EventLogEntry, 'id' | 'timestamp'>) => void;
  activeRoute: RouteOption;
}

const SimulationContext = createContext<SimulationContextValue | null>(null);

export const SimulationProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [mode, setMode] = useState<OperationMode>('SIMULATION');
  const [state, setState] = useState<SimulationState>(createInitialSimulationState);
  const [wsStatus, setWsStatus] = useState<WebSocketStatus>('DISCONNECTED');
  const smartDemoTimerRef = useRef<number | null>(null);

  const logEvent = useCallback((entry: Omit<EventLogEntry, 'id' | 'timestamp'>) => {
    const newEvent: EventLogEntry = {
      id: `EVT-${Date.now().toString().slice(-4)}`,
      timestamp: new Date().toLocaleTimeString(),
      ...entry,
    };
    setState((prev) => ({
      ...prev,
      events: [newEvent, ...prev.events.slice(0, 199)],
    }));
  }, []);

  // Monitor WebSocket Status
  useEffect(() => {
    const unsub = wsService.onStatusChange((status) => {
      setWsStatus(status);
      if (status === 'CONNECTED' && mode === 'LIVE') {
        logEvent({
          source: 'SYSTEM',
          level: 'SUCCESS',
          message: 'Connected to SMART AEGIS Coordinator WebSocket bridge.',
        });
      }
    });
    return unsub;
  }, [mode, logEvent]);

  // Connect / Disconnect WS when switching modes
  useEffect(() => {
    if (mode === 'LIVE') {
      wsService.connect();
      const unsubMessage = wsService.subscribe((msg: any) => {
        if (msg.command === 'INTERSECTION_READY' || msg.command === 'ACK') {
          setState((prev) => {
            const nextIntersections = prev.intersections.map((int) => {
              if (int.id === msg.intersection) {
                return {
                  ...int,
                  health: 'ONLINE' as const,
                  blueLed: msg.details?.includes('BLUE=ON') ?? int.blueLed,
                  redLed: msg.details?.includes('RED=ON') ?? int.redLed,
                };
              }
              return int;
            });
            return { ...prev, intersections: nextIntersections };
          });
        }
      });
      return () => {
        unsubMessage();
        wsService.disconnect();
      };
    } else {
      wsService.disconnect();
    }
  }, [mode]);

  // Periodic IRSA re-evaluation loop (runs every 1000ms)
  useEffect(() => {
    const irsaTimer = window.setInterval(() => {
      setState((prev) => {
        const nodeHealthMap = new Map<number, string>();
        prev.intersections.forEach((n) => nodeHealthMap.set(n.id, n.health));

        const evaluatedRoads = prev.roads.map((road) => {
          const n1Health = nodeHealthMap.get(road.fromNodeId) || 'ONLINE';
          const n2Health = nodeHealthMap.get(road.toNodeId) || 'ONLINE';
          return evaluateRoadIRSA(road, prev.weather, n1Health, n2Health);
        });

        // Recalculate routes to the target hospital's nearest node
        const targetNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
        const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, evaluatedRoads, 1, targetNodeId);

        return {
          ...prev,
          roads: evaluatedRoads,
          routes: newRoutes,
        };
      });
    }, 1000);

    return () => clearInterval(irsaTimer);
  }, []);

  // Main Simulation Clock Tick (runs every 100ms)
  useEffect(() => {
    if (mode === 'LIVE') return;

    const interval = window.setInterval(() => {
      setState((prev) => {
        // 1. Advance signal countdowns on all 12 nodes
        let nextIntersections = prev.intersections.map((int) => {
          if (int.preempted) return int;
          let nextCount = int.countdown - 1;
          let nextColor = int.signalColor;
          if (nextCount <= 0) {
            if (nextColor === 'GREEN') {
              nextColor = 'YELLOW';
              nextCount = 4;
            } else if (nextColor === 'YELLOW') {
              nextColor = 'RED';
              nextCount = 20;
            } else {
              nextColor = 'GREEN';
              nextCount = 25;
            }
          }
          return { ...int, countdown: nextCount, signalColor: nextColor };
        });

        if (!prev.isRunning || prev.isPaused) {
          return { ...prev, intersections: nextIntersections };
        }

        // 2. Active Ambulance Motion along the active route
        let speed = prev.ambulance.speedKmh;
        let pos = prev.ambulance.positionPercent;
        let road = prev.ambulance.currentRoad;
        let eta = prev.ambulance.etaSeconds;

        if (prev.ambulance.isBreakdown) {
          return {
            ...prev,
            ambulance: { ...prev.ambulance, speedKmh: 0 },
            intersections: nextIntersections,
          };
        }

        // Apply weather speed cap
        let maxSpeed = 75;
        if (prev.weather.mode === 'RAIN') maxSpeed = 50;
        if (prev.weather.mode === 'FOG') maxSpeed = 40;
        if (prev.heavyTraffic) maxSpeed = Math.min(maxSpeed, 45);

        speed = Math.min(maxSpeed, speed + 3);

        const step = (speed / 3600) * 10 * 1.8;
        pos = Math.min(100, pos + step);

        // Preempt intersections along the active route
        const activeRouteObj = prev.routes[
          prev.activeRouteId === 'PRIMARY'
            ? 'primaryRoute'
            : prev.activeRouteId === 'ALT_A'
            ? 'altRouteA'
            : 'altRouteB'
        ];
        const routeNodes = activeRouteObj.nodeIds;

        // Calculate which node the ambulance is currently approaching
        const totalSegments = Math.max(1, routeNodes.length - 1);
        const currentSegmentIndex = Math.min(totalSegments - 1, Math.floor((pos / 100) * totalSegments));
        const currentNodeApproaching = routeNodes[currentSegmentIndex + 1] || routeNodes[routeNodes.length - 1];

        // Update green preemption for current corridor node
        nextIntersections = nextIntersections.map((int) => {
          const isCurrentApproaching = int.id === currentNodeApproaching;
          if (isCurrentApproaching && pos < 98) {
            return {
              ...int,
              blueLed: true,
              redLed: false,
              signalColor: 'GREEN',
              preempted: true,
              oledStatus: 'EMERGENCY PREEMPTION ACTIVE',
              irSensorTriggered: true,
            };
          } else if (int.preempted && !isCurrentApproaching) {
            // Releasing node
            return {
              ...int,
              blueLed: false,
              preempted: false,
              oledStatus: 'NORMAL RESUMED',
              irSensorTriggered: false,
            };
          }
          return int;
        });

        // Current road name
        if (pos < 30) {
          road = 'MG Road Corridor';
          eta = Math.round(36 * (1 - pos / 100));
        } else if (pos < 65) {
          road = prev.activeRouteId === 'ALT_A' ? 'Queens Road Bypass' : 'Residency Cross Expressway';
          eta = Math.round(24 * (1 - pos / 100));
        } else if (pos < 98) {
          road = prev.activeRouteId === 'ALT_B' ? 'Airport Expressway Incline' : 'Richmond Circle Flyover';
          eta = Math.round(10 * (1 - pos / 100));
        } else {
          pos = 100;
          speed = 0;
          road = prev.hospital.name + ' Emergency Bay';
          eta = 0;
          nextIntersections = nextIntersections.map((int) => ({
            ...int,
            blueLed: false,
            preempted: false,
            irSensorTriggered: false,
            oledStatus: 'NORMAL RESUMED',
          }));
        }

        const isComplete = pos >= 100;

        return {
          ...prev,
          isRunning: !isComplete,
          intersections: nextIntersections,
          ambulance: {
            ...prev.ambulance,
            speedKmh: Math.round(speed),
            positionPercent: Math.round(pos * 10) / 10,
            currentRoad: road,
            currentSegmentIndex,
            nextIntersectionId: currentNodeApproaching,
            etaSeconds: eta,
            isActive: !isComplete,
            patientCondition: isComplete ? 'Admitted to ICU' : 'Triage Monitored / Stable',
          },
          hospital: {
            ...prev.hospital,
            receivingTeamAlerted: pos > 10 && !isComplete,
            traumaBaysAvailable: isComplete ? Math.max(0, prev.hospital.traumaBaysAvailable - 1) : prev.hospital.traumaBaysAvailable,
          },
          metrics: {
            ...prev.metrics,
            totalMissions: isComplete ? prev.metrics.totalMissions + 1 : prev.metrics.totalMissions,
            successfulMissions: isComplete
              ? prev.metrics.successfulMissions + 1
              : prev.metrics.successfulMissions,
          },
          replayHistory: [
            ...prev.replayHistory,
            {
              timestamp: Date.now(),
              ambulancePos: pos,
              intersections: nextIntersections,
              speed,
            },
          ].slice(-500),
        };
      });
    }, 100);

    return () => clearInterval(interval);
  }, [mode]);

  const startMission = useCallback(
    (priority: EmergencyPriority = 'P1_CRITICAL') => {
      setState((prev) => ({
        ...prev,
        isRunning: true,
        isPaused: false,
        stage: 1,
        ambulance: {
          ...prev.ambulance,
          speedKmh: 15,
          positionPercent: 0,
          currentRoad: 'Station Alpha West Exit',
          etaSeconds: 38,
          priority,
          isActive: true,
          isBreakdown: false,
          patientCondition: 'Acute Cardiac Distress (Priority 1)',
        },
      }));
      logEvent({
        source: 'COORDINATOR',
        level: 'EMERGENCY',
        message: `EMERGENCY ALERT: Priority ${priority} -> Green Corridor Preempted across City Network.`,
        details: 'Dynamic Dijkstra evaluated Primary Route via Intersections [1, 2, 3]',
      });
    },
    [logEvent]
  );

  const stopMission = useCallback(() => {
    setState((prev) => ({
      ...prev,
      isRunning: false,
      isPaused: false,
      ambulance: { ...prev.ambulance, isActive: false, speedKmh: 0 },
      intersections: prev.intersections.map((int) => ({
        ...int,
        blueLed: false,
        preempted: false,
        oledStatus: 'NORMAL RESUMED',
      })),
    }));
    logEvent({
      source: 'COORDINATOR',
      level: 'WARN',
      message: 'Mission aborted by operator. All corridor locks safely released.',
    });
  }, [logEvent]);

  const pauseSimulation = useCallback(() => {
    setState((prev) => ({ ...prev, isPaused: true }));
    logEvent({ source: 'SYSTEM', level: 'INFO', message: 'Simulation paused.' });
  }, [logEvent]);

  const resumeSimulation = useCallback(() => {
    setState((prev) => ({ ...prev, isPaused: false }));
    logEvent({ source: 'SYSTEM', level: 'INFO', message: 'Simulation resumed.' });
  }, [logEvent]);

  const resetSimulation = useCallback(() => {
    if (smartDemoTimerRef.current) {
      clearInterval(smartDemoTimerRef.current);
      smartDemoTimerRef.current = null;
    }
    setState(createInitialSimulationState());
    logEvent({
      source: 'SYSTEM',
      level: 'INFO',
      message: 'IRSA City Grid & Simulation fully reset to nominal baseline.',
    });
  }, [logEvent]);

  // Fault Injections
  const togglePoliceOverride = useCallback(() => {
    setState((prev) => {
      const active = !prev.policeOverride;
      logEvent({
        source: 'POLICE',
        level: active ? 'EMERGENCY' : 'INFO',
        message: active
          ? 'POLICE OVERRIDE ACTIVATED: All cross-traffic halted at Node 1, 2, 3.'
          : 'Police override deactivated. Standard preemption restored.',
      });
      return {
        ...prev,
        policeOverride: active,
        policeEscort: { active, unitId: 'PATROL-01' },
      };
    });
  }, [logEvent]);

  const toggleRoadBlocked = useCallback((roadId: string = 'ROAD-01') => {
    setState((prev) => {
      const targetRoad = prev.roads.find((r) => r.id === roadId);
      const blocked = targetRoad ? targetRoad.roadCondition !== 'BLOCKED' : !prev.roadBlocked;
      const updatedRoads = prev.roads.map((r) =>
        r.id === roadId
          ? {
              ...r,
              roadCondition: (blocked ? 'BLOCKED' : 'EXCELLENT') as any,
              incident: (blocked ? 'ROAD_CLOSED' : 'NORMAL') as any,
              congestionPercent: blocked ? 98 : 28,
              isBlocked: blocked,
            }
          : r
      );

      const targetNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
      const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, updatedRoads, 1, targetNodeId);

      logEvent({
        source: 'IRSA_ENGINE',
        level: blocked ? 'EMERGENCY' : 'INFO',
        message: blocked
          ? `HAZARD INJECTED: ${roadId} completely blocked. Dynamic reroute triggered!`
          : `${roadId} cleared. Normal primary corridor restored.`,
        details: blocked ? 'Auto-switching to Alternative Corridor A (Queens Road Bypass)' : undefined,
      });

      return {
        ...prev,
        roadBlocked: blocked,
        roads: updatedRoads,
        routes: newRoutes,
        activeRouteId: blocked ? 'ALT_A' : 'PRIMARY',
      };
    });
  }, [logEvent]);

  const toggleHeavyTraffic = useCallback(() => {
    setState((prev) => {
      const heavy = !prev.heavyTraffic;
      const updatedRoads = prev.roads.map((r) =>
        r.id === 'ROAD-02' || r.id === 'ROAD-05'
          ? {
              ...r,
              congestionPercent: heavy ? 92 : 34,
              trafficDensity: (heavy ? 'RED' : 'GREEN') as any,
            }
          : r
      );

      const targetNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
      const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, updatedRoads, 1, targetNodeId);

      logEvent({
        source: 'IRSA_ENGINE',
        level: heavy ? 'WARN' : 'INFO',
        message: heavy
          ? 'TRAFFIC CONGESTION SPIKE: Residency Cross & Koramangala Ringway at 92% capacity.'
          : 'Traffic normalized across arterial network.',
      });

      return {
        ...prev,
        heavyTraffic: heavy,
        roads: updatedRoads,
        routes: newRoutes,
      };
    });
  }, [logEvent]);

  const toggleAccident = useCallback((roadId: string = 'ROAD-02') => {
    setState((prev) => {
      const targetRoad = prev.roads.find((r) => r.id === roadId);
      const accident = targetRoad ? targetRoad.incident !== 'ACCIDENT' : !prev.accidentOnRoute;
      const updatedRoads = prev.roads.map((r) =>
        r.id === roadId
          ? {
              ...r,
              roadCondition: (accident ? 'DAMAGED' : 'GOOD') as any,
              incident: (accident ? 'ACCIDENT' : 'NORMAL') as any,
              accidentProbability: accident ? 85 : 6,
              congestionPercent: accident ? 85 : 34,
              hasAccident: accident,
            }
          : r
      );

      const targetNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
      const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, updatedRoads, 1, targetNodeId);

      logEvent({
        source: 'IRSA_ENGINE',
        level: accident ? 'EMERGENCY' : 'INFO',
        message: accident
          ? `INCIDENT DETECTED: 2-car collision on ${roadId}. Rerouting around crash site.`
          : 'Accident scene cleared by police recovery team.',
        details: accident ? 'Alternative Corridor A activated via Node 10 & 11' : undefined,
      });

      return {
        ...prev,
        accidentOnRoute: accident,
        roads: updatedRoads,
        routes: newRoutes,
        activeRouteId: accident ? 'ALT_A' : 'PRIMARY',
      };
    });
  }, [logEvent]);

  const toggleFlood = useCallback((roadId?: string) => {
    setState((prev) => {
      const targetId = roadId || 'ROAD-04';
      const targetRoad = prev.roads.find((r) => r.id === targetId);
      const flood = targetRoad ? !targetRoad.floodStatus : !prev.floodActive;
      const updatedRoads = prev.roads.map((r) =>
        (roadId ? r.id === roadId : r.id === 'ROAD-04' || r.id === 'ROAD-15')
          ? {
              ...r,
              floodStatus: flood,
              roadCondition: (flood ? 'WET' : 'GOOD') as any,
              congestionPercent: flood ? 75 : 32,
              isFlooded: flood,
            }
          : r
      );
      logEvent({
        source: 'IRSA_ENGINE',
        level: flood ? 'WARN' : 'INFO',
        message: flood
          ? 'FLASH FLOOD ALERT: Underpass waterlogging on road network.'
          : 'Water cleared by municipal drainage pumps.',
      });
      return { ...prev, floodActive: flood, roads: updatedRoads };
    });
  }, [logEvent]);

  const toggleConstruction = useCallback(() => {
    setState((prev) => {
      const c = !prev.constructionActive;
      const updatedRoads = prev.roads.map((r) =>
        r.id === 'ROAD-03'
          ? { ...r, constructionStatus: c, congestionPercent: c ? 60 : 25 }
          : r
      );
      logEvent({
        source: 'IRSA_ENGINE',
        level: c ? 'WARN' : 'INFO',
        message: c
          ? 'ROADWORKS REPORTED: Lane closure for metro utility work on ROAD-03.'
          : 'Roadworks finished. Full dual-lane reopened.',
      });
      return { ...prev, constructionActive: c, roads: updatedRoads };
    });
  }, [logEvent]);

  const toggleBridgeClosed = useCallback(() => {
    setState((prev) => {
      const b = !prev.bridgeClosed;
      const updatedRoads = prev.roads.map((r) =>
        r.id === 'ROAD-21'
          ? {
              ...r,
              roadCondition: (b ? 'BLOCKED' : 'EXCELLENT') as any,
              incident: (b ? 'BRIDGE_CLOSED' : 'NORMAL') as any,
            }
          : r
      );
      logEvent({
        source: 'IRSA_ENGINE',
        level: b ? 'WARN' : 'INFO',
        message: b
          ? 'INFRASTRUCTURE ADVISORY: Halasuru Lake Overpass Bridge closed for structural inspection.'
          : 'Bridge reopened to normal transit.',
      });
      return { ...prev, bridgeClosed: b, roads: updatedRoads };
    });
  }, [logEvent]);

  const toggleCameraFailure = useCallback(() => {
    setState((prev) => {
      const fail = !prev.cameraFailure;
      const updatedRoads = prev.roads.map((r) =>
        r.id === 'ROAD-02' && r.camera
          ? {
              ...r,
              camera: {
                ...r.camera,
                health: (fail ? 'OFFLINE' : 'ONLINE') as any,
                fps: fail ? 0 : 30,
              },
            }
          : r
      );
      logEvent({
        source: 'CAMERA_MESH',
        level: fail ? 'ERROR' : 'SUCCESS',
        message: fail
          ? 'CAMERA SENSOR OFFLINE: CAM-02 (Residency North-Cam) lost video feed. IRSA confidence lowered.'
          : 'CAM-02 reconnected. AI edge vehicle count operational at 30 FPS.',
      });
      return { ...prev, cameraFailure: fail, roads: updatedRoads };
    });
  }, [logEvent]);

  // Node Failure Recovery (Part 4)
  const toggleNodeFailure = useCallback(
    (targetNodeId: number = 2) => {
      setState((prev) => {
        const targetNode = prev.intersections.find((n) => n.id === targetNodeId);
        const willFail = targetNode?.health === 'ONLINE';

        const updatedIntersections = prev.intersections.map((n) =>
          n.id === targetNodeId ? { ...n, health: (willFail ? 'ERROR' : 'ONLINE') as any } : n
        );

        // Update road weights connecting to this dead node
        const nodeHealthMap = new Map<number, string>();
        updatedIntersections.forEach((n) => nodeHealthMap.set(n.id, n.health));

        const updatedRoads = prev.roads.map((road) => {
          const n1Health = nodeHealthMap.get(road.fromNodeId) || 'ONLINE';
          const n2Health = nodeHealthMap.get(road.toNodeId) || 'ONLINE';
          return evaluateRoadIRSA(road, prev.weather, n1Health, n2Health);
        });

        const targetHospitalNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
        const newRoutes = calculateMultiCriteriaRoutes(updatedIntersections, updatedRoads, 1, targetHospitalNodeId);

        const alertText = willFail
          ? `NODE FAILURE DETECTED: Node #${targetNodeId} Offline -> Re-routing... Alternative Corridor Activated -> Mission Continues`
          : null;

        logEvent({
          source: `NODE_${targetNodeId}`,
          level: willFail ? 'ERROR' : 'SUCCESS',
          message: willFail
            ? `CRITICAL NODE DROP: Intersection ${targetNodeId} dropped off TCP socket mesh.`
            : `Intersection ${targetNodeId} restored and synchronized with Central Coordinator.`,
          details: willFail
            ? 'Dynamic graph recomputed: Rerouting emergency vehicle via bypass corridor.'
            : undefined,
        });

        return {
          ...prev,
          intersectionFailed: willFail,
          intersections: updatedIntersections,
          roads: updatedRoads,
          routes: newRoutes,
          activeRouteId: willFail ? 'ALT_A' : 'PRIMARY',
          nodeFailureAlert: alertText,
        };
      });
    },
    [logEvent]
  );

  const toggleAmbulanceBreakdown = useCallback(() => {
    setState((prev) => {
      const b = !prev.ambulance.isBreakdown;
      logEvent({
        source: 'AMBULANCE',
        level: b ? 'ERROR' : 'SUCCESS',
        message: b
          ? 'VEHICLE BREAKDOWN: AMB-101 motor fault. Emergency backup AMB-102 dispatched from Station Beta.'
          : 'AMB-101 motor restart successful.',
      });
      return {
        ...prev,
        ambulance: { ...prev.ambulance, isBreakdown: b },
      };
    });
  }, [logEvent]);

  const toggleHospitalBusy = useCallback(() => {
    setState((prev) => {
      const busy = !prev.hospitalBusy;
      const targetHospital = busy
        ? {
            id: 'HOSP-02',
            name: 'Apollo Specialty Hospital',
            traumaBaysAvailable: 2,
            totalTraumaBays: 5,
            orReady: true,
            erWaitMinutes: 5,
            receivingTeamAlerted: true,
            status: 'GREEN' as const,
          }
        : {
            id: 'HOSP-01',
            name: 'City General Hospital',
            traumaBaysAvailable: 4,
            totalTraumaBays: 6,
            orReady: true,
            erWaitMinutes: 4,
            receivingTeamAlerted: true,
            status: 'GREEN' as const,
          };

      // Recalculate routes to new destination node
      const targetNodeId = busy ? 8 : 3;
      const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, prev.roads, 1, targetNodeId);

      logEvent({
        source: 'HOSPITAL',
        level: busy ? 'WARN' : 'INFO',
        message: busy
          ? 'HOSPITAL FULL CAPACITY: City General Trauma bays occupied. Dynamic diversion to Apollo Specialty!'
          : 'City General Trauma Bay reopened.',
        details: busy ? 'Rerouting corridor toward Node 8 (Old Airport Expressway)' : undefined,
      });

      return {
        ...prev,
        hospitalBusy: busy,
        hospital: targetHospital,
        routes: newRoutes,
        ambulance: {
          ...prev.ambulance,
          destination: targetHospital.name,
          targetHospitalId: targetHospital.id,
        },
      };
    });
  }, [logEvent]);

  const setWeather = useCallback(
    (w: WeatherCondition['mode']) => {
      setState((prev) => {
        let vis = 100;
        let fric = 0.95;
        if (w === 'NIGHT') vis = 70;
        if (w === 'RAIN') {
          vis = 60;
          fric = 0.7;
        }
        if (w === 'FOG') {
          vis = 40;
          fric = 0.85;
        }

        const nextWeather: WeatherCondition = {
          mode: w,
          visibilityPercent: vis,
          frictionIndex: fric,
        };

        const updatedRoads = prev.roads.map((r) => evaluateRoadIRSA(r, nextWeather));
        const targetNodeId = prev.hospital.id === 'HOSP-02' ? 8 : 3;
        const newRoutes = calculateMultiCriteriaRoutes(prev.intersections, updatedRoads, 1, targetNodeId);

        logEvent({
          source: 'METEOROLOGICAL',
          level: 'INFO',
          message: `Atmospheric condition changed to: ${w} (Visibility: ${vis}%, Friction: ${fric})`,
        });

        return {
          ...prev,
          weather: nextWeather,
          roads: updatedRoads,
          routes: newRoutes,
        };
      });
    },
    [logEvent]
  );

  const selectRoad = useCallback((roadId: string | null) => {
    setState((prev) => ({ ...prev, selectedRoadId: roadId, selectedNodeId: null }));
  }, []);

  const selectNode = useCallback((nodeId: number | null) => {
    setState((prev) => ({ ...prev, selectedNodeId: nodeId, selectedRoadId: null }));
  }, []);

  const setActiveRoute = useCallback(
    (routeId: 'PRIMARY' | 'ALT_A' | 'ALT_B') => {
      setState((prev) => {
        logEvent({
          source: 'COORDINATOR',
          level: 'INFO',
          message: `Manual corridor switch: Now navigating via ${prev.routes[routeId === 'PRIMARY' ? 'primaryRoute' : routeId === 'ALT_A' ? 'altRouteA' : 'altRouteB'].name}`,
        });
        return { ...prev, activeRouteId: routeId };
      });
    },
    [logEvent]
  );

  // Judge "Smart Demo" Orchestrator (Part 9)
  const startSmartDemo = useCallback(() => {
    resetSimulation();
    setState((prev) => ({
      ...prev,
      smartDemoActive: true,
      smartDemoStep: 1,
      smartDemoNarration: 'Step 1/8: Initializing Normal Emergency Mission (AMB-101 Dispatched)',
    }));

    // Step 1: Start Normal Mission
    startMission('P1_CRITICAL');

    // Step 2 at 4 seconds: Traffic Spikes on Primary Corridor
    setTimeout(() => {
      toggleHeavyTraffic();
      setState((prev) => ({
        ...prev,
        smartDemoStep: 2,
        smartDemoNarration: 'Step 2/8: Traffic density spikes on central arterial. IRSA evaluating congestion.',
      }));
    }, 4000);

    // Step 3 at 8 seconds: Road Block on MG Road -> Automatic Reroute to Alt Route A
    setTimeout(() => {
      toggleRoadBlocked();
      setState((prev) => ({
        ...prev,
        smartDemoStep: 3,
        smartDemoNarration: 'Step 3/8: MG Road blocked by obstruction. Dynamic Dijkstra automatically rerouting around hazard!',
      }));
    }, 8000);

    // Step 4 at 12 seconds: Hospital Capacity Alert -> Target switches to Apollo Trauma
    setTimeout(() => {
      toggleHospitalBusy();
      setState((prev) => ({
        ...prev,
        smartDemoStep: 4,
        smartDemoNarration: 'Step 4/8: City General trauma bays full. Automated diversion to Apollo Specialty Hospital.',
      }));
    }, 12000);

    // Step 5 at 16 seconds: Node 2 Failure -> Recovery Corridor Activated
    setTimeout(() => {
      toggleNodeFailure(2);
      setState((prev) => ({
        ...prev,
        smartDemoStep: 5,
        smartDemoNarration: 'Step 5/8: NODE FAILURE DETECTED (Node 2 dropped). Autonomous fail-safe bypass corridor engaged!',
      }));
    }, 16000);

    // Step 6 at 20 seconds: Heavy Rain Weather Injected -> Speed throttled
    setTimeout(() => {
      setWeather('RAIN');
      setState((prev) => ({
        ...prev,
        smartDemoStep: 6,
        smartDemoNarration: 'Step 6/8: Heavy rain weather penalty applied. Vehicle speed automatically throttled for safety.',
      }));
    }, 20000);

    // Step 7 at 25 seconds: Approaching Hospital
    setTimeout(() => {
      setState((prev) => ({
        ...prev,
        smartDemoStep: 7,
        smartDemoNarration: 'Step 7/8: Final corridor preemption at Node 8 (Airport Expressway). Trauma bay ready.',
      }));
    }, 25000);

    // Step 8 at 28 seconds: Mission Success
    setTimeout(() => {
      setState((prev) => ({
        ...prev,
        smartDemoStep: 8,
        smartDemoNarration: 'Step 8/8: MISSION SUCCESS! Patient admitted to Apollo ICU. Corridor returned to municipal cycle.',
      }));
    }, 28000);
  }, [resetSimulation, startMission, toggleHeavyTraffic, toggleRoadBlocked, toggleHospitalBusy, toggleNodeFailure, setWeather]);

  const stopSmartDemo = useCallback(() => {
    setState((prev) => ({
      ...prev,
      smartDemoActive: false,
      smartDemoStep: 0,
      smartDemoNarration: null,
    }));
  }, []);

  const activeRoute = state.routes[
    state.activeRouteId === 'PRIMARY'
      ? 'primaryRoute'
      : state.activeRouteId === 'ALT_A'
      ? 'altRouteA'
      : 'altRouteB'
  ];

  return (
    <SimulationContext.Provider
      value={{
        mode,
        setMode,
        state,
        wsStatus,
        startMission,
        stopMission,
        pauseSimulation,
        resumeSimulation,
        resetSimulation,
        togglePoliceOverride,
        toggleRoadBlocked,
        toggleHeavyTraffic,
        toggleAccident,
        toggleFlood,
        toggleConstruction,
        toggleBridgeClosed,
        toggleCameraFailure,
        toggleNodeFailure,
        toggleIntersectionFailure: toggleNodeFailure,
        toggleAmbulanceBreakdown,
        toggleHospitalBusy,
        setWeather,
        selectRoad,
        selectNode,
        setActiveRoute,
        selectRoute: setActiveRoute,
        startSmartDemo,
        stopSmartDemo,
        logEvent,
        activeRoute,
      }}
    >
      {children}
    </SimulationContext.Provider>
  );
};

export const useSimulationContext = () => {
  const ctx = useContext(SimulationContext);
  if (!ctx) {
    throw new Error('useSimulationContext must be used within a SimulationProvider');
  }
  return ctx;
};
