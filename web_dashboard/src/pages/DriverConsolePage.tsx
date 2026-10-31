/**
 * @file DriverConsolePage.tsx
 * @brief Autonomous Ambulance Driver Dashboard & In-Vehicle Telematics Console.
 * Modern automotive dark-theme UI with large touch controls, live GPS map,
 * real-time animated ECG vitals, 4-channel camera HUD, voice guidance, and dynamic rerouting.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { useSimulation } from '../hooks/useSimulation';
import { AutomotiveCluster } from '../components/DriverConsole/AutomotiveCluster';
import { LiveDriverMap } from '../components/DriverConsole/LiveDriverMap';
import { PatientMonitorHUD } from '../components/DriverConsole/PatientMonitorHUD';
import { AmbulanceCameraView } from '../components/DriverConsole/AmbulanceCameraView';
import { VoiceGuidanceHUD } from '../components/DriverConsole/VoiceGuidanceHUD';
import { DynamicAlertsBanner } from '../components/DriverConsole/DynamicAlertsBanner';
import { DriverAlert } from '../types';
import { voiceNav } from '../services/voiceNavigationService';
import {
  Navigation,
  Play,
  Pause,
  RotateCcw,
  Shuffle,
  ShieldCheck,
  AlertTriangle,
  Zap,
} from 'lucide-react';

export const DriverConsolePage: React.FC = () => {
  const {
    state,
    startMission,
    stopMission,
    pauseSimulation,
    resumeSimulation,
    activeRoute,
    toggleNodeFailure,
    toggleRoadBlocked,
    logEvent,
  } = useSimulation();

  const [alerts, setAlerts] = useState<DriverAlert[]>([
    {
      id: 'ALT-INIT',
      type: 'POLICE_ESCORT',
      title: 'POLICE ESCORT STAGED',
      message: 'Cruiser PATROL-01 assigned to lead corridor through Downtown.',
      severity: 'INFO',
      timestamp: new Date().toLocaleTimeString(),
      active: true,
    },
  ]);

  // Compute dynamic next turn guidance based on ambulance progress
  const getNextTurnInfo = () => {
    const pos = state.ambulance.positionPercent;
    if (pos < 25) {
      return {
        text: 'Continue straight through Intersection 1 (Main & 1st Ave)',
        distance: Math.max(50, Math.round(350 * (1 - pos / 25))),
        direction: 'STRAIGHT' as const,
      };
    } else if (pos < 60) {
      if (state.activeRouteId === 'ALT_A') {
        return {
          text: 'Turn Left onto Queens Road Bypass to avoid Node 2 drop',
          distance: Math.max(50, Math.round(400 * (1 - (pos - 25) / 35))),
          direction: 'LEFT' as const,
        };
      }
      return {
        text: 'Turn Right onto Residency Road towards Intersection 2',
        distance: Math.max(50, Math.round(400 * (1 - (pos - 25) / 35))),
        direction: 'RIGHT' as const,
      };
    } else if (pos < 95) {
      return {
        text: 'Proceed across Hospital Approach Flyover to Trauma Bay',
        distance: Math.max(50, Math.round(300 * (1 - (pos - 60) / 35))),
        direction: 'STRAIGHT' as const,
      };
    } else {
      return {
        text: 'Arriving at City General Hospital Emergency Trauma Bay',
        distance: 0,
        direction: 'STRAIGHT' as const,
      };
    }
  };

  const turnInfo = getNextTurnInfo();

  // Watch for Node Failure in simulation state and push an immediate dynamic alert + speech
  useEffect(() => {
    if (state.intersectionFailed || state.activeRouteId === 'ALT_A') {
      const exists = alerts.some((a) => a.type === 'ALTERNATIVE_ROUTE');
      if (!exists) {
        const rerouteAlert: DriverAlert = {
          id: `ALT-REROUTE-${Date.now()}`,
          type: 'ALTERNATIVE_ROUTE',
          title: 'DYNAMIC ROUTE RECALCULATED',
          message: 'Node 2 Conflict Detected. Queens Road Bypass engaged. ETA: 28s.',
          severity: 'CRITICAL',
          timestamp: new Date().toLocaleTimeString(),
          active: true,
        };
        setAlerts((prev) => [rerouteAlert, ...prev]);
        voiceNav.speak('Alternative Route Found. Queens Road Bypass engaged. New ETA 28 seconds.', 'HIGH');
      }
    }
  }, [state.intersectionFailed, state.activeRouteId]);

  const handleDismissAlert = (id: string) => {
    setAlerts((prev) => prev.filter((a) => a.id !== id));
  };

  const handleTriggerAlert = (type: DriverAlert['type']) => {
    const newAlert: DriverAlert = {
      id: `ALT-MANUAL-${Date.now()}`,
      type,
      title: `${type.replace(/_/g, ' ')} ALERT`,
      message: `Tactical alert triggered from In-Vehicle Telematics Console at ${new Date().toLocaleTimeString()}`,
      severity: type === 'ROAD_CLOSED' || type === 'NODE_FAILURE' ? 'CRITICAL' : 'WARNING',
      timestamp: new Date().toLocaleTimeString(),
      active: true,
    };
    setAlerts((prev) => [newAlert, ...prev]);
  };

  const handleSimulateReroute = () => {
    // Feature 3: Trigger dynamic reroute without stopping the mission
    toggleNodeFailure(2);
    logEvent({
      source: 'DRIVER_CONSOLE',
      level: 'WARN',
      message: 'TACTICAL REROUTE: In-vehicle console requested bypass around Intersection 2.',
    });
  };

  return (
    <div className="space-y-5">
      {/* Driver Console Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 pb-3 border-b border-gray-800">
        <div className="flex items-center space-x-3">
          <div className="p-2.5 rounded-xl bg-cyan-500/10 text-cyan-400 border border-cyan-500/30 shadow-[0_0_15px_rgba(0,240,255,0.2)]">
            <Navigation size={22} />
          </div>
          <div>
            <h1 className="text-xl font-black text-white font-mono tracking-wider uppercase flex items-center space-x-2">
              <span>Ambulance Driver Console</span>
              <span className="text-[10px] px-2 py-0.5 rounded bg-cyan-500/20 text-cyan-300 border border-cyan-400/40">
                TACTICAL HUD
              </span>
            </h1>
            <p className="text-xs text-gray-400 font-mono">
              Vehicle AMB-101 • Unit Aegis Guardian 1 • Green Corridor Active
            </p>
          </div>
        </div>

        {/* Quick Driver Action Controls */}
        <div className="flex items-center space-x-2 font-mono text-xs">
          {!state.isRunning ? (
            <button
              onClick={() => startMission()}
              className="px-4 py-2 rounded-xl bg-gradient-to-r from-emerald-600 to-teal-600 hover:from-emerald-500 hover:to-teal-500 text-white font-bold flex items-center space-x-1.5 shadow-[0_0_15px_rgba(16,185,129,0.3)] transition"
            >
              <Play size={14} />
              <span>Engage Mission</span>
            </button>
          ) : state.isPaused ? (
            <button
              onClick={resumeSimulation}
              className="px-4 py-2 rounded-xl bg-cyan-600 hover:bg-cyan-500 text-white font-bold flex items-center space-x-1.5 transition"
            >
              <Play size={14} />
              <span>Resume</span>
            </button>
          ) : (
            <button
              onClick={pauseSimulation}
              className="px-4 py-2 rounded-xl bg-amber-600 hover:bg-amber-500 text-white font-bold flex items-center space-x-1.5 transition"
            >
              <Pause size={14} />
              <span>Pause</span>
            </button>
          )}

          <button
            onClick={handleSimulateReroute}
            className="px-3.5 py-2 rounded-xl bg-slate-900 border border-slate-700 hover:border-cyan-500 text-cyan-300 font-bold flex items-center space-x-1.5 transition shadow-sm"
            title="Simulate node failure and instant dynamic route recalculation"
          >
            <Shuffle size={14} className="text-cyan-400" />
            <span className="hidden sm:inline">Test Reroute</span>
          </button>

          {state.isRunning && (
            <button
              onClick={stopMission}
              className="px-3 py-2 rounded-xl bg-slate-900 border border-rose-500/40 text-rose-400 hover:bg-rose-950/40 font-bold transition"
            >
              <RotateCcw size={14} />
            </button>
          )}
        </div>
      </div>

      {/* Main Automotive Cluster (Speedometer, Turn-by-Turn, Telematics) */}
      <AutomotiveCluster
        ambulance={state.ambulance}
        nextTurnText={turnInfo.text}
        nextTurnDistanceMeters={turnInfo.distance}
        turnDirection={turnInfo.direction}
      />

      {/* Tactical Flash Alerts Banner */}
      <DynamicAlertsBanner
        alerts={alerts}
        onDismissAlert={handleDismissAlert}
        onTriggerAlert={handleTriggerAlert}
      />

      {/* Dual Column Layout: Navigation Map (Left 7 cols) & Perception/Vitals (Right 5 cols) */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-5">
        {/* Left Column: Live Tactical Driver Map & Voice Guidance (7 cols) */}
        <div className="lg:col-span-7 space-y-5">
          <LiveDriverMap
            intersections={state.intersections}
            roads={state.roads}
            activeRoute={activeRoute}
            ambulance={state.ambulance}
          />

          <VoiceGuidanceHUD />
        </div>

        {/* Right Column: Real-Time ECG Patient Monitor & 4-Channel Video Cams (5 cols) */}
        <div className="lg:col-span-5 space-y-5">
          <PatientMonitorHUD ambulance={state.ambulance} />

          <AmbulanceCameraView speedKmh={state.ambulance.speedKmh} />
        </div>
      </div>
    </div>
  );
};
