import React from 'react';
import {
  Play,
  Square,
  Pause,
  RotateCcw,
  ShieldAlert,
  Flame,
  AlertTriangle,
  Car,
  Wrench,
  ServerOff,
  Hospital,
  Sun,
  Moon,
  CloudRain,
  CloudFog,
  Sliders,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const ControlPanel: React.FC = () => {
  const {
    state,
    startMission,
    stopMission,
    pauseSimulation,
    resumeSimulation,
    resetSimulation,
    togglePoliceOverride,
    toggleRoadBlocked,
    toggleHeavyTraffic,
    toggleAccident,
    toggleAmbulanceBreakdown,
    toggleIntersectionFailure,
    toggleHospitalBusy,
    setWeather,
  } = useSimulation();

  return (
    <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-6">
      {/* Title */}
      <div className="flex items-center justify-between border-b border-gray-800/80 pb-3">
        <div className="flex items-center space-x-2">
          <Sliders className="w-4 h-4 text-cyan-400" />
          <h3 className="font-heading-cyber font-bold text-white text-base tracking-wide">
            MISSION & SCENARIO CONTROL MATRIX
          </h3>
        </div>
        <span className="text-[11px] font-mono px-2 py-0.5 rounded bg-gray-800 text-gray-400 border border-gray-700">
          QNX Interoperability Ready
        </span>
      </div>

      {/* Primary Lifecycle Controls */}
      <div>
        <span className="text-xs font-semibold text-gray-400 uppercase tracking-wider block mb-2.5">
          1. Mission Lifecycle
        </span>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
          <button
            onClick={() => startMission('P1_CRITICAL')}
            disabled={state.isRunning}
            className={`flex items-center justify-center space-x-2 px-3 py-2.5 rounded-lg text-xs font-semibold shadow-lg transition-all ${
              state.isRunning
                ? 'bg-gray-800 text-gray-600 border border-gray-700 cursor-not-allowed'
                : 'btn-cyber-emergency'
            }`}
          >
            <Flame className="w-4 h-4" />
            <span>Start Emergency</span>
          </button>

          <button
            onClick={stopMission}
            disabled={!state.isRunning}
            className={`flex items-center justify-center space-x-2 px-3 py-2.5 rounded-lg text-xs font-semibold transition-all ${
              !state.isRunning
                ? 'bg-gray-800 text-gray-600 border border-gray-700 cursor-not-allowed'
                : 'bg-rose-950/50 hover:bg-rose-900/60 text-rose-300 border border-rose-600/40'
            }`}
          >
            <Square className="w-4 h-4" />
            <span>Abort Mission</span>
          </button>

          <button
            onClick={state.isPaused ? resumeSimulation : pauseSimulation}
            disabled={!state.isRunning}
            className={`flex items-center justify-center space-x-2 px-3 py-2.5 rounded-lg text-xs font-semibold border transition-all ${
              !state.isRunning
                ? 'bg-gray-800 text-gray-600 border-gray-700 cursor-not-allowed'
                : state.isPaused
                ? 'bg-amber-950/50 text-amber-300 border-amber-500/50 animate-pulse'
                : 'bg-gray-800 hover:bg-gray-700 text-gray-200 border-gray-700'
            }`}
          >
            {state.isPaused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
            <span>{state.isPaused ? 'Resume' : 'Pause'}</span>
          </button>

          <button
            onClick={resetSimulation}
            className="flex items-center justify-center space-x-2 px-3 py-2.5 rounded-lg text-xs font-medium bg-gray-800/80 hover:bg-gray-700 text-gray-300 border border-gray-700 transition-all"
          >
            <RotateCcw className="w-4 h-4" />
            <span>Reset Standby</span>
          </button>
        </div>
      </div>

      {/* Dynamic Scenario Hazard Injections */}
      <div>
        <span className="text-xs font-semibold text-gray-400 uppercase tracking-wider block mb-2.5">
          2. Fault Injection & Field Hazards
        </span>
        <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-2">
          {/* Police Override */}
          <button
            onClick={togglePoliceOverride}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.policeOverride
                ? 'bg-blue-950/80 border-blue-500 text-blue-300 shadow-[0_0_12px_rgba(59,130,246,0.3)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <ShieldAlert className="w-5 h-5 mb-1.5" />
            <span>Police Override</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.policeOverride ? 'ACTIVE' : 'OFF'}
            </span>
          </button>

          {/* Road Blocked */}
          <button
            onClick={() => toggleRoadBlocked()}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.roadBlocked
                ? 'bg-amber-950/80 border-amber-500 text-amber-300 shadow-[0_0_12px_rgba(245,158,11,0.3)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <AlertTriangle className="w-5 h-5 mb-1.5" />
            <span>Road Blocked</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.roadBlocked ? 'REPLAN' : 'CLEAR'}
            </span>
          </button>

          {/* Heavy Traffic */}
          <button
            onClick={toggleHeavyTraffic}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.heavyTraffic
                ? 'bg-rose-950/80 border-rose-500 text-rose-300 shadow-[0_0_12px_rgba(239,68,68,0.3)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <Car className="w-5 h-5 mb-1.5" />
            <span>Heavy Traffic</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.heavyTraffic ? '+5s Ext' : 'NORMAL'}
            </span>
          </button>

          {/* Accident */}
          <button
            onClick={() => toggleAccident()}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.accidentOnRoute
                ? 'bg-red-950/80 border-red-500 text-red-300 shadow-[0_0_12px_rgba(239,68,68,0.4)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <AlertTriangle className="w-5 h-5 mb-1.5" />
            <span>Accident Scene</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.accidentOnRoute ? 'BYPASS' : 'NONE'}
            </span>
          </button>

          {/* Ambulance Breakdown */}
          <button
            onClick={toggleAmbulanceBreakdown}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.ambulance.isBreakdown
                ? 'bg-purple-950/80 border-purple-500 text-purple-300 shadow-[0_0_12px_rgba(168,85,247,0.4)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <Wrench className="w-5 h-5 mb-1.5" />
            <span>AMB Breakdown</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.ambulance.isBreakdown ? 'BACKUP' : 'NOMINAL'}
            </span>
          </button>

          {/* Node Failure */}
          <button
            onClick={() => toggleIntersectionFailure()}
            className={`flex flex-col items-center justify-center p-3 rounded-lg border text-xs font-medium transition-all ${
              state.intersectionFailed
                ? 'bg-red-950/80 border-red-500 text-red-300 shadow-[0_0_12px_rgba(239,68,68,0.4)]'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:border-gray-700 hover:text-gray-200'
            }`}
          >
            <ServerOff className="w-5 h-5 mb-1.5" />
            <span>Node 2 Failure</span>
            <span className="text-[10px] font-mono mt-0.5 text-gray-500">
              {state.intersectionFailed ? 'FAIL-SAFE' : 'ONLINE'}
            </span>
          </button>
        </div>
      </div>

      {/* Atmospheric / Weather Controls */}
      <div>
        <span className="text-xs font-semibold text-gray-400 uppercase tracking-wider block mb-2.5">
          3. Environmental Conditions
        </span>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-2.5">
          <button
            onClick={() => setWeather('CLEAR')}
            className={`flex items-center space-x-2 px-3 py-2 rounded-lg border text-xs font-medium transition-all ${
              state.weather.mode === 'CLEAR'
                ? 'bg-cyan-500/20 border-cyan-400 text-cyan-300'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:text-gray-200'
            }`}
          >
            <Sun className="w-4 h-4 text-amber-400" />
            <span>Clear Daylight</span>
          </button>

          <button
            onClick={() => setWeather('NIGHT')}
            className={`flex items-center space-x-2 px-3 py-2 rounded-lg border text-xs font-medium transition-all ${
              state.weather.mode === 'NIGHT'
                ? 'bg-cyan-500/20 border-cyan-400 text-cyan-300'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:text-gray-200'
            }`}
          >
            <Moon className="w-4 h-4 text-indigo-400" />
            <span>Night Operation</span>
          </button>

          <button
            onClick={() => setWeather('RAIN')}
            className={`flex items-center space-x-2 px-3 py-2 rounded-lg border text-xs font-medium transition-all ${
              state.weather.mode === 'RAIN'
                ? 'bg-cyan-500/20 border-cyan-400 text-cyan-300'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:text-gray-200'
            }`}
          >
            <CloudRain className="w-4 h-4 text-blue-400" />
            <span>Precipitation (Rain)</span>
          </button>

          <button
            onClick={() => setWeather('FOG')}
            className={`flex items-center space-x-2 px-3 py-2 rounded-lg border text-xs font-medium transition-all ${
              state.weather.mode === 'FOG'
                ? 'bg-cyan-500/20 border-cyan-400 text-cyan-300'
                : 'bg-gray-900/80 border-gray-800 text-gray-400 hover:text-gray-200'
            }`}
          >
            <CloudFog className="w-4 h-4 text-gray-400" />
            <span>Dense Fog / Haze</span>
          </button>
        </div>
      </div>
    </div>
  );
};
