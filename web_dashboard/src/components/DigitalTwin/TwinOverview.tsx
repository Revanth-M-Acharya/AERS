import React from 'react';
import { Cpu, Radio, Network, CheckCircle2, RefreshCw, Layers } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const TwinOverview: React.FC = () => {
  const { state, mode } = useSimulation();

  return (
    <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between pb-3 border-b border-gray-800">
        <div className="flex items-center space-x-2.5">
          <Cpu className="w-5 h-5 text-cyan-400" />
          <h3 className="font-heading-cyber font-bold text-white text-base">
            DIGITAL TWIN SYNCHRONIZATION ENGINE
          </h3>
        </div>
        <div className="flex items-center space-x-2 text-xs font-mono text-cyan-400">
          <RefreshCw className="w-3.5 h-3.5 animate-spin" />
          <span>Bi-directional Sync (100 Hz)</span>
        </div>
      </div>

      {/* Synchronized Components Grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* Node 1 Twin */}
        <div className="bg-gray-900/80 p-4 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center justify-between text-xs">
            <span className="font-bold text-gray-200">Intersection 1 Twin</span>
            <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
              SYNCED
            </span>
          </div>
          <div className="text-[11px] font-mono space-y-1 text-gray-400">
            <div className="flex justify-between">
              <span>Target Hardware:</span>
              <span className="text-gray-200">Pi 2 (10.204.179.245)</span>
            </div>
            <div className="flex justify-between">
              <span>Virtual Blue LED:</span>
              <span className={state.intersections[0].blueLed ? 'text-cyan-400 font-bold' : 'text-gray-500'}>
                {state.intersections[0].blueLed ? 'HIGH (3.3V)' : 'LOW (0V)'}
              </span>
            </div>
            <div className="flex justify-between">
              <span>Virtual Red LED:</span>
              <span className={state.intersections[0].redLed ? 'text-rose-400 font-bold' : 'text-gray-500'}>
                {state.intersections[0].redLed ? 'HIGH (3.3V)' : 'LOW (0V)'}
              </span>
            </div>
          </div>
        </div>

        {/* Node 2 Twin */}
        <div className="bg-gray-900/80 p-4 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center justify-between text-xs">
            <span className="font-bold text-gray-200">Intersection 2 Twin</span>
            <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
              SYNCED
            </span>
          </div>
          <div className="text-[11px] font-mono space-y-1 text-gray-400">
            <div className="flex justify-between">
              <span>Target Hardware:</span>
              <span className="text-gray-200">Pi 3 (10.204.179.130)</span>
            </div>
            <div className="flex justify-between">
              <span>Virtual Blue LED:</span>
              <span className={state.intersections[1].blueLed ? 'text-cyan-400 font-bold' : 'text-gray-500'}>
                {state.intersections[1].blueLed ? 'HIGH (3.3V)' : 'LOW (0V)'}
              </span>
            </div>
            <div className="flex justify-between">
              <span>Virtual Red LED:</span>
              <span className={state.intersections[1].redLed ? 'text-rose-400 font-bold' : 'text-gray-500'}>
                {state.intersections[1].redLed ? 'HIGH (3.3V)' : 'LOW (0V)'}
              </span>
            </div>
          </div>
        </div>

        {/* Node 3 Twin (Shortage Model) */}
        <div className="bg-gray-900/80 p-4 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center justify-between text-xs">
            <span className="font-bold text-gray-200">Intersection 3 Twin</span>
            <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-amber-500/20 text-amber-300">
              SHORTAGE RULE
            </span>
          </div>
          <div className="text-[11px] font-mono space-y-1 text-gray-400">
            <div className="flex justify-between">
              <span>Target Hardware:</span>
              <span className="text-gray-200">Pi 4 (10.204.179.144)</span>
            </div>
            <div className="flex justify-between">
              <span>Virtual Blue LED:</span>
              <span className={state.intersections[2].blueLed ? 'text-cyan-400 font-bold' : 'text-gray-500'}>
                {state.intersections[2].blueLed ? 'HIGH (3.3V)' : 'LOW (0V)'}
              </span>
            </div>
            <div className="flex justify-between">
              <span>Red LED Capability:</span>
              <span className="text-gray-500 italic">No-Op Virtualized</span>
            </div>
          </div>
        </div>

        {/* Master Coordinator Twin */}
        <div className="bg-gray-900/80 p-4 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center justify-between text-xs">
            <span className="font-bold text-gray-200">Coordinator Twin</span>
            <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-400">
              ACTIVE
            </span>
          </div>
          <div className="text-[11px] font-mono space-y-1 text-gray-400">
            <div className="flex justify-between">
              <span>Hardware Isolation:</span>
              <span className="text-cyan-300">Pure Software</span>
            </div>
            <div className="flex justify-between">
              <span>GPIO Access:</span>
              <span className="text-emerald-400 font-bold">ZERO (Enforced)</span>
            </div>
            <div className="flex justify-between">
              <span>Mesh Sockets:</span>
              <span className="text-gray-200">3 Open TCP Sockets</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
