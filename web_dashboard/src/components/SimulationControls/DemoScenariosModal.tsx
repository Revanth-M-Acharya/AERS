import React, { useState } from 'react';
import {
  X,
  Play,
  RotateCcw,
  ShieldAlert,
  AlertTriangle,
  CloudRain,
  CloudFog,
  ServerCrash,
  Radio,
  Flame,
  Shield,
  Users,
  GitMerge,
  Cpu,
  RefreshCw,
  History,
  CheckCircle2,
  Sparkles,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

interface DemoScenariosModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export interface ScenarioDefinition {
  id: number;
  code: string;
  title: string;
  category: 'CORE' | 'FAILURE' | 'ENVIRONMENT' | 'PRIORITY' | 'AUDIT';
  badge: string;
  description: string;
  qnxBehavior: string;
  icon: React.ElementType;
  accentColor: string;
}

export const DEMO_SCENARIOS: ScenarioDefinition[] = [
  {
    id: 1,
    code: 'SCEN_01',
    title: 'Normal Emergency Dispatch',
    category: 'CORE',
    badge: 'Baseline Green Wave',
    description: 'P1 Critical cardiac emergency dispatched from Depot 1 to City Trauma Center. Zero stops, full all-red conflict lockdown engaged.',
    qnxBehavior: 'QNX Coordinator issues PREEMPT_GREEN packet over TCP; RPi 2 & 3 fire GPIO 6 (Blue LED) within 0.8ms.',
    icon: Flame,
    accentColor: 'rose',
  },
  {
    id: 2,
    code: 'SCEN_02',
    title: 'Node Failure & Autonomous Reroute',
    category: 'FAILURE',
    badge: 'Failover Dijkstra',
    description: 'Intersection Node #2 crashes mid-transit. Watchdog detects missed heartbeats; dynamic Dijkstra computes Queens Road bypass without coordinator reboot.',
    qnxBehavior: 'Pulse timer triggers heartbeat expiration after 1500ms; QNX state machine shifts route and updates client nodes.',
    icon: ServerCrash,
    accentColor: 'amber',
  },
  {
    id: 3,
    code: 'SCEN_03',
    title: 'Peak Traffic Jam Surge',
    category: 'ENVIRONMENT',
    badge: 'Queue Dissipation',
    description: 'Residency road congestion spikes to 95%. Continuous ETA updates and corridor extension clears upstream vehicle queue before ambulance arrival.',
    qnxBehavior: 'Continuous ETA engine recalculates traversal velocity based on IRSA density vectors and extends preemption window.',
    icon: AlertTriangle,
    accentColor: 'yellow',
  },
  {
    id: 4,
    code: 'SCEN_04',
    title: 'Hospital ICU Saturation Divert',
    category: 'CORE',
    badge: 'Dynamic Diversion',
    description: 'City Trauma Center reaches 100% bed capacity. System autonomously redirects ambulance to St. Jude Heart Institute with updated corridor lock.',
    qnxBehavior: 'HospitalSelectionEngine matches specialty capabilities, queries bed availability, and updates corridor waypoints in real time.',
    icon: ShieldAlert,
    accentColor: 'purple',
  },
  {
    id: 5,
    code: 'SCEN_05',
    title: 'Road Blockage & Camera AI Flag',
    category: 'FAILURE',
    badge: 'IRSA Obstruction Avoidance',
    description: 'Sudden obstacle on MG Road detected by traffic AI camera. IRSA suitability drops to 0; automated bypass route activated.',
    qnxBehavior: 'CameraPerception event received over QNX message passing; incident detection engine drops edge weight and reroutes.',
    icon: AlertTriangle,
    accentColor: 'rose',
  },
  {
    id: 6,
    code: 'SCEN_06',
    title: 'Heavy Rainfall Friction Degradation',
    category: 'ENVIRONMENT',
    badge: 'Adverse Weather Control',
    description: 'Torrential downpour lowers road friction coefficient. Safe vehicle speed cap applied (40 km/h) and yellow clearance extended from 3s to 5s.',
    qnxBehavior: 'SafetyPolicyEngine enforces conservative deceleration curves and extends clearance pulses in QnxTimerService.',
    icon: CloudRain,
    accentColor: 'cyan',
  },
  {
    id: 7,
    code: 'SCEN_07',
    title: 'Dense Fog & Optical Beacon Assist',
    category: 'ENVIRONMENT',
    badge: 'Visual Fog Safety',
    description: 'Visibility drops below 50m. Camera perception confidence falls; system engages maximum intensity blue strobe beacons for driver guidance.',
    qnxBehavior: 'OpticalSensorFilter switches to conservative cross-traffic confirmation; activates auxiliary GPIO warning strobes.',
    icon: CloudFog,
    accentColor: 'blue',
  },
  {
    id: 8,
    code: 'SCEN_08',
    title: 'Coordinator Failover & Edge Recovery',
    category: 'FAILURE',
    badge: 'Distributed RTOS Fallback',
    description: 'Coordinator link momentarily drops. Edge nodes transition to autonomous fixed-time cycles, then seamlessly re-sync when connection restores.',
    qnxBehavior: 'TrafficNodeRuntime detects socket disconnect; invokes NodeRecoveryManager to keep physical signals in fail-safe cycle.',
    icon: RefreshCw,
    accentColor: 'emerald',
  },
  {
    id: 9,
    code: 'SCEN_09',
    title: 'ESP32 Wireless Node Disconnect',
    category: 'FAILURE',
    badge: 'Wireless Mesh Fallback',
    description: 'Wireless edge sensor node drops offline. Cluster supervisor identifies missing telemetry and gracefully switches to timed progression.',
    qnxBehavior: 'ClusterHealthMonitor logs missing node; executes fallback green-wave timeline without crashing the cluster.',
    icon: Radio,
    accentColor: 'orange',
  },
  {
    id: 10,
    code: 'SCEN_10',
    title: 'Priority Preemption Override',
    category: 'PRIORITY',
    badge: 'P1 Overrides P3',
    description: 'P1 Critical ambulance overrides and pre-empts lower P3 Medium vehicle crossing the shared intersection hub.',
    qnxBehavior: 'IntelligentPriorityScheduler evaluates clinical triage weights, resolving signal preemption grant in favor of P1.',
    icon: Shield,
    accentColor: 'rose',
  },
  {
    id: 11,
    code: 'SCEN_11',
    title: 'Police Tactical Override',
    category: 'PRIORITY',
    badge: 'Manual Authority Override',
    description: 'Police manual command console locks Brigade Road junction to ALL-RED for VIP escort/tactical incident.',
    qnxBehavior: 'ManualOverrideManager processes high-priority command frame; overrides automated green corridor with security lock.',
    icon: Shield,
    accentColor: 'blue',
  },
  {
    id: 12,
    code: 'SCEN_12',
    title: 'Mass Casualty Incident (MCI)',
    category: 'PRIORITY',
    badge: 'Multi-Hospital Distribution',
    description: 'Multi-patient incident; priority scheduler orchestrates simultaneous corridor reservations and bed allocations across all 3 trauma centers.',
    qnxBehavior: 'FleetManagementEngine orchestrates multi-ambulance dispatch vectors while coordinating simultaneous ICU reservations.',
    icon: Users,
    accentColor: 'amber',
  },
  {
    id: 13,
    code: 'SCEN_13',
    title: 'Multiple Ambulances Crossing Conflict',
    category: 'PRIORITY',
    badge: 'Conflict Detection & Deconfliction',
    description: 'Two ambulances converge on intersecting paths. Conflict detection engine staggers green waves to prevent collision and deadlock.',
    qnxBehavior: 'ConflictDetectionEngine calculates ETA arrival delta; grants corridor window sequentially to resolve overlapping reservation.',
    icon: GitMerge,
    accentColor: 'cyan',
  },
  {
    id: 14,
    code: 'SCEN_14',
    title: 'Live IRSA Dynamic Route Switch',
    category: 'CORE',
    badge: 'Multi-Criteria Re-optimization',
    description: 'Real-time multi-criteria scoring continuously compares Primary vs Alternative routes; switches recommended path as conditions evolve.',
    qnxBehavior: 'RoadConfidenceEngine continuously computes 5-factor IRSA vector, streaming updated recommendations to Digital Twin.',
    icon: Cpu,
    accentColor: 'purple',
  },
  {
    id: 15,
    code: 'SCEN_15',
    title: 'Black-Box Mission Replay',
    category: 'AUDIT',
    badge: 'Post-Mission Telemetry Replay',
    description: 'Plays back recorded black-box mission timeline with step-by-step vehicle velocity, green corridor state, and audit logs.',
    qnxBehavior: 'SimulationRecorder / MissionReplayEngine feeds logged telemetry frames into the event bridge for post-incident review.',
    icon: History,
    accentColor: 'emerald',
  },
];

export const DemoScenariosModal: React.FC<DemoScenariosModalProps> = ({ isOpen, onClose }) => {
  const {
    startMission,
    resetSimulation,
    toggleNodeFailure,
    toggleHeavyTraffic,
    toggleHospitalBusy,
    toggleRoadBlocked,
    setWeather,
    togglePoliceOverride,
    selectRoute,
    logEvent,
  } = useSimulation();

  const [activeCategory, setActiveCategory] = useState<string>('ALL');
  const [runningScenario, setRunningScenario] = useState<number | null>(null);

  if (!isOpen) return null;

  const handleLaunchScenario = (scen: ScenarioDefinition) => {
    setRunningScenario(scen.id);
    resetSimulation();

    setTimeout(() => {
      switch (scen.id) {
        case 1: // Normal Emergency
          startMission('P1_CRITICAL');
          break;
        case 2: // Node Failure
          startMission('P1_CRITICAL');
          setTimeout(() => toggleNodeFailure(2), 2000);
          break;
        case 3: // Traffic Jam
          toggleHeavyTraffic();
          startMission('P1_CRITICAL');
          break;
        case 4: // Hospital Full
          toggleHospitalBusy();
          startMission('P1_CRITICAL');
          break;
        case 5: // Road Block
          toggleRoadBlocked('ROAD-01');
          startMission('P1_CRITICAL');
          break;
        case 6: // Rain
          setWeather('RAIN');
          startMission('P2_HIGH');
          break;
        case 7: // Fog
          setWeather('FOG');
          startMission('P1_CRITICAL');
          break;
        case 8: // Coordinator Recovery
          startMission('P1_CRITICAL');
          logEvent({
            source: 'QNX_SUPERVISOR',
            level: 'WARN',
            message: 'SIMULATION: Coordinator primary socket dropped; edge nodes entering fallback cycle.',
          });
          setTimeout(() => {
            logEvent({
              source: 'QNX_SUPERVISOR',
              level: 'SUCCESS',
              message: 'SIMULATION: Coordinator reconnected. Signal synchronization re-acquired.',
            });
          }, 3500);
          break;
        case 9: // ESP32 Offline
          toggleNodeFailure(3);
          startMission('P2_HIGH');
          break;
        case 10: // Priority Override
          startMission('P1_CRITICAL');
          logEvent({
            source: 'PRIORITY_SCHEDULER',
            level: 'INFO',
            message: 'AMB-101 (P1 Critical) given preemption priority over AMB-104 (P3 Medium).',
          });
          break;
        case 11: // Police Override
          togglePoliceOverride();
          startMission('P1_CRITICAL');
          break;
        case 12: // Mass Casualty
          startMission('P1_CRITICAL');
          toggleHospitalBusy();
          logEvent({
            source: 'DISPATCH_ENGINE',
            level: 'EMERGENCY',
            message: 'MASS CASUALTY INCIDENT (MCI Level 2) declared. Fleet management activating 3 corridors.',
          });
          break;
        case 13: // Multiple Ambulances
          startMission('P1_CRITICAL');
          logEvent({
            source: 'CONFLICT_DETECTOR',
            level: 'WARN',
            message: 'Convergence warning: AMB-101 and AMB-102 crossing INT_02 window. Staggering clearance phases.',
          });
          break;
        case 14: // IRSA Route Switch
          toggleHeavyTraffic();
          selectRoute('ALT_A');
          startMission('P1_CRITICAL');
          break;
        case 15: // Mission Replay
          startMission('P1_CRITICAL');
          logEvent({
            source: 'REPLAY_ENGINE',
            level: 'INFO',
            message: 'Black-box flight recorder active. Logging high-resolution GPS & signal states.',
          });
          break;
        default:
          startMission('P1_CRITICAL');
          break;
      }

      logEvent({
        source: 'DEMO_ORCHESTRATOR',
        level: 'SUCCESS',
        message: `DEMO SCENARIO ${scen.id} ACTIVATED: ${scen.title}`,
        details: scen.qnxBehavior,
      });

      onClose();
    }, 300);
  };

  const filteredScenarios =
    activeCategory === 'ALL'
      ? DEMO_SCENARIOS
      : DEMO_SCENARIOS.filter((s) => s.category === activeCategory);

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-md animate-fade-in">
      <div className="bg-[#0B101D] border border-cyan-500/40 rounded-2xl w-full max-w-5xl max-h-[90vh] flex flex-col shadow-2xl shadow-cyan-950/40 overflow-hidden">
        {/* Header */}
        <div className="p-5 border-b border-gray-800 flex items-center justify-between bg-gradient-to-r from-gray-950 via-[#0B101D] to-gray-950">
          <div className="flex items-center space-x-3">
            <div className="p-2.5 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600 text-black shadow-lg shadow-cyan-500/20">
              <Sparkles size={22} className="font-bold" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h2 className="text-lg font-extrabold text-white tracking-wide">
                  HACKATHON DEMONSTRATION SUITE
                </h2>
                <span className="px-2 py-0.5 text-[10px] font-mono font-bold bg-cyan-500/20 text-cyan-300 rounded border border-cyan-500/40">
                  15 ONE-CLICK SCENARIOS
                </span>
              </div>
              <p className="text-xs text-gray-400 mt-0.5">
                Targeted stress-testing scenarios demonstrating QNX RTOS deterministic preemption, IRSA rerouting, and fail-safe recovery
              </p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-2 text-gray-400 hover:text-white rounded-lg hover:bg-gray-800/60 transition"
          >
            <X size={20} />
          </button>
        </div>

        {/* Filter Tabs */}
        <div className="px-5 py-3 border-b border-gray-800/80 bg-gray-950/40 flex items-center space-x-2 overflow-x-auto text-xs font-mono">
          {['ALL', 'CORE', 'FAILURE', 'ENVIRONMENT', 'PRIORITY', 'AUDIT'].map((cat) => (
            <button
              key={cat}
              onClick={() => setActiveCategory(cat)}
              className={`px-3 py-1.5 rounded-lg border transition ${
                activeCategory === cat
                  ? 'bg-cyan-500/20 border-cyan-500/50 text-cyan-300 font-bold'
                  : 'bg-gray-900/40 border-gray-800 text-gray-400 hover:text-gray-200'
              }`}
            >
              {cat}
            </button>
          ))}
        </div>

        {/* Scenarios Grid */}
        <div className="p-5 overflow-y-auto grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3.5">
          {filteredScenarios.map((scen) => {
            const Icon = scen.icon;
            const isRunning = runningScenario === scen.id;

            return (
              <div
                key={scen.id}
                className={`p-4 rounded-xl border flex flex-col justify-between transition-all duration-200 hover:scale-[1.01] ${
                  isRunning
                    ? 'bg-cyan-950/40 border-cyan-400 shadow-lg shadow-cyan-950/50'
                    : 'bg-gray-900/50 border-gray-800 hover:border-gray-700 hover:bg-gray-900/80'
                }`}
              >
                <div>
                  <div className="flex items-center justify-between mb-2">
                    <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-gray-800 text-cyan-400 font-bold border border-gray-700">
                      {scen.code}
                    </span>
                    <span className="text-[10px] font-mono font-medium px-2 py-0.5 rounded bg-gray-800/60 text-gray-400">
                      {scen.badge}
                    </span>
                  </div>

                  <div className="flex items-start space-x-2.5 mb-2">
                    <div className="p-2 rounded-lg bg-gray-800 text-cyan-300 flex-shrink-0 mt-0.5">
                      <Icon size={16} />
                    </div>
                    <div>
                      <h4 className="text-sm font-bold text-white leading-snug">{scen.title}</h4>
                      <p className="text-xs text-gray-400 mt-1 line-clamp-3 leading-relaxed">
                        {scen.description}
                      </p>
                    </div>
                  </div>

                  <div className="bg-gray-950/60 border border-gray-800 rounded p-2 my-2 text-[11px] font-mono text-gray-400">
                    <span className="text-cyan-400 font-semibold">QNX RTOS: </span>
                    {scen.qnxBehavior}
                  </div>
                </div>

                <button
                  onClick={() => handleLaunchScenario(scen)}
                  className="mt-2 w-full py-2 bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-black font-extrabold rounded-lg text-xs font-mono tracking-wider flex items-center justify-center space-x-2 shadow-md shadow-cyan-950/40 transition"
                >
                  <Play size={12} fill="currentColor" />
                  <span>LAUNCH SCENARIO</span>
                </button>
              </div>
            );
          })}
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-gray-800 bg-gray-950/60 flex items-center justify-between text-xs text-gray-400">
          <div className="flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span>Ready for Live Hackathon Presentation & Video Demonstration</span>
          </div>
          <button
            onClick={onClose}
            className="px-4 py-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-lg text-xs font-medium"
          >
            Close
          </button>
        </div>
      </div>
    </div>
  );
};
