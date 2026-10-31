import React, { useState, useEffect } from 'react';
import {
  Activity,
  Flame,
  Radio,
  RotateCcw,
  Wifi,
  WifiOff,
  CloudSun,
  ShieldCheck,
  Clock,
  Layers,
  Sparkles,
} from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { DemoScenariosModal } from './SimulationControls/DemoScenariosModal';

export const TopHeader: React.FC = () => {
  const { mode, setMode, state, startMission, resetSimulation, wsStatus } = useSimulation();
  const [timeStr, setTimeStr] = useState<string>('');
  const [isDemoModalOpen, setIsDemoModalOpen] = useState<boolean>(false);

  useEffect(() => {
    const updateTime = () => {
      const now = new Date();
      setTimeStr(now.toTimeString().split(' ')[0] + ' UTC');
    };
    updateTime();
    const interval = setInterval(updateTime, 1000);
    return () => clearInterval(interval);
  }, []);

  const onlineNodesCount = state.intersections.filter((i) => i.health === 'ONLINE').length;

  return (
    <header className="h-16 bg-[#0D121F]/95 backdrop-blur-md border-b border-gray-800 px-6 flex items-center justify-between z-20">
      {/* Left: Quick System Telemetry */}
      <div className="flex items-center space-x-6 text-xs">
        {/* Coordinator Pill */}
        <div className="flex items-center space-x-2 bg-gray-900/80 border border-gray-800 px-3 py-1.5 rounded-md">
          <Radio className="w-3.5 h-3.5 text-cyan-400" />
          <span className="text-gray-400">Coordinator:</span>
          <span className="font-mono font-bold text-emerald-400">ONLINE</span>
        </div>

        {/* Nodes Health */}
        <div className="flex items-center space-x-2 bg-gray-900/80 border border-gray-800 px-3 py-1.5 rounded-md">
          <Layers className="w-3.5 h-3.5 text-blue-400" />
          <span className="text-gray-400">Traffic Nodes:</span>
          <span className="font-mono font-bold text-white">
            <span className="text-emerald-400">{onlineNodesCount}</span> / 3 Active
          </span>
        </div>

        {/* Live / Sim Indicator */}
        <div className="flex items-center space-x-2">
          {mode === 'LIVE' ? (
            <div className="flex items-center space-x-1.5 text-emerald-400 bg-emerald-950/40 border border-emerald-500/30 px-3 py-1.5 rounded-md">
              <Wifi className="w-3.5 h-3.5 animate-pulse" />
              <span className="font-mono font-semibold text-[11px]">QNX CLUSTER LIVE</span>
            </div>
          ) : (
            <div className="flex items-center space-x-1.5 text-cyan-400 bg-cyan-950/40 border border-cyan-500/30 px-3 py-1.5 rounded-md">
              <Activity className="w-3.5 h-3.5" />
              <span className="font-mono font-semibold text-[11px]">STANDALONE SIMULATION</span>
            </div>
          )}
        </div>

        {/* Active Emergency Indicator */}
        {state.isRunning && (
          <div className="flex items-center space-x-2 bg-rose-950/60 border border-rose-500/40 text-rose-400 px-3 py-1.5 rounded-md animate-pulse">
            <Flame className="w-3.5 h-3.5 text-rose-400" />
            <span className="font-mono font-bold text-[11px]">PRIORITY CORRIDOR ACTIVE</span>
          </div>
        )}
      </div>

      {/* Right: Quick Controls & Clock */}
      <div className="flex items-center space-x-4">
        {/* Mode Switcher Toggle */}
        <div className="flex items-center bg-gray-900 border border-gray-800 rounded-lg p-0.5 text-xs">
          <button
            onClick={() => setMode('SIMULATION')}
            className={`px-3 py-1 rounded-md font-medium transition-all ${
              mode === 'SIMULATION'
                ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 font-semibold'
                : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            Simulation
          </button>
          <button
            onClick={() => setMode('LIVE')}
            className={`px-3 py-1 rounded-md font-medium transition-all ${
              mode === 'LIVE'
                ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 font-semibold'
                : 'text-gray-400 hover:text-gray-200'
            }`}
          >
            Live QNX
          </button>
        </div>

        {/* 15 Hackathon Scenarios Button */}
        <button
          onClick={() => setIsDemoModalOpen(true)}
          className="flex items-center space-x-1.5 px-3 py-1.5 rounded-md text-xs font-bold bg-gradient-to-r from-cyan-500/20 to-blue-500/20 hover:from-cyan-500/30 hover:to-blue-500/30 text-cyan-300 border border-cyan-500/40 shadow-sm transition-all"
        >
          <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
          <span>15 Scenarios</span>
        </button>

        {/* Action: Trigger Emergency */}
        <button
          onClick={() => startMission('P1_CRITICAL')}
          disabled={state.isRunning}
          className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-md text-xs font-semibold shadow-md transition-all ${
            state.isRunning
              ? 'bg-gray-800 text-gray-500 cursor-not-allowed border border-gray-700'
              : 'bg-rose-600 hover:bg-rose-500 text-white shadow-rose-900/40 hover:shadow-rose-600/50'
          }`}
        >
          <Flame className="w-3.5 h-3.5" />
          <span>Start Emergency</span>
        </button>

        {/* Action: Reset */}
        <button
          onClick={resetSimulation}
          className="flex items-center space-x-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 px-3 py-1.5 rounded-md text-xs font-medium border border-gray-700 transition-all"
        >
          <RotateCcw className="w-3.5 h-3.5" />
          <span>Reset</span>
        </button>

        {/* Clock */}
        <div className="flex items-center space-x-1.5 text-gray-400 font-mono text-xs pl-2 border-l border-gray-800">
          <Clock className="w-3.5 h-3.5 text-gray-500" />
          <span>{timeStr || '10:00:00 UTC'}</span>
        </div>
      </div>

      {/* 15 Scenarios Modal */}
      <DemoScenariosModal
        isOpen={isDemoModalOpen}
        onClose={() => setIsDemoModalOpen(false)}
      />
    </header>
  );
};
