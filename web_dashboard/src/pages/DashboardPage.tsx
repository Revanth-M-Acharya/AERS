import React from 'react';
import {
  Activity,
  Flame,
  Radio,
  Clock,
  Layers,
  TrendingDown,
  ShieldAlert,
  Zap,
} from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { InteractiveCityMap } from '../components/CityMap/InteractiveCityMap';
import { ControlPanel } from '../components/SimulationControls/ControlPanel';
import { SignalClusterView } from '../components/TrafficSignals/SignalClusterView';
import { AmbulanceTelemetry } from '../components/Ambulance/AmbulanceTelemetry';
import { HospitalStatusCard } from '../components/Hospital/HospitalStatusCard';
import { LiveEventTable } from '../components/EventLog/LiveEventTable';

export const DashboardPage: React.FC = () => {
  const { state } = useSimulation();

  return (
    <div className="space-y-6">
      {/* Top Telemetry KPI Cards */}
      <div className="grid grid-cols-2 md:grid-cols-4 lg:grid-cols-6 gap-3.5">
        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <Radio className="w-3.5 h-3.5 text-cyan-400" />
            <span>Coordinator</span>
          </div>
          <div className="font-mono text-base font-bold text-emerald-400">ONLINE</div>
          <div className="text-[10px] text-gray-500 font-mono">Pi 1: 10.204.179.4</div>
        </div>

        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <Layers className="w-3.5 h-3.5 text-blue-400" />
            <span>Traffic Nodes</span>
          </div>
          <div className="font-mono text-base font-bold text-white">
            {state.intersections.filter((i) => i.health === 'ONLINE').length} / {state.intersections.length} Online
          </div>
          <div className="text-[10px] text-gray-500 font-mono">12-Node City Mesh</div>
        </div>

        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <Flame className="w-3.5 h-3.5 text-rose-400" />
            <span>Corridor Status</span>
          </div>
          <div
            className={`font-mono text-base font-bold truncate ${
              state.isRunning ? 'text-cyan-400 animate-pulse' : 'text-gray-400'
            }`}
          >
            {state.isRunning ? 'PREEMPTION' : 'STANDBY'}
          </div>
          <div className="text-[10px] text-gray-500 font-mono">Dijkstra Dynamic</div>
        </div>

        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <Clock className="w-3.5 h-3.5 text-emerald-400" />
            <span>Target ETA</span>
          </div>
          <div className="font-mono text-base font-bold text-emerald-400">
            {state.ambulance.etaSeconds} sec
          </div>
          <div className="text-[10px] text-gray-500 font-mono">Bay 4 Reservation</div>
        </div>

        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <Zap className="w-3.5 h-3.5 text-amber-400" />
            <span>Mesh Latency</span>
          </div>
          <div className="font-mono text-base font-bold text-white">
            {state.metrics.latencyMs} ms
          </div>
          <div className="text-[10px] text-gray-500 font-mono">124 pkts/sec</div>
        </div>

        <div className="glass-panel p-3.5 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-1.5 text-xs text-gray-400 mb-1">
            <TrendingDown className="w-3.5 h-3.5 text-indigo-400" />
            <span>Success Rate</span>
          </div>
          <div className="font-mono text-base font-bold text-indigo-300">
            100% ({state.metrics.successfulMissions} Msn)
          </div>
          <div className="text-[10px] text-gray-500 font-mono">-61% Transit Delay</div>
        </div>
      </div>

      {/* Main Grid: Interactive Map & Telemetry Panels */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Left Column (2 Cols): City Map & Simulation Scenario Matrix */}
        <div className="lg:col-span-2 space-y-6">
          <div className="h-[440px]">
            <InteractiveCityMap />
          </div>
          <ControlPanel />
        </div>

        {/* Right Column (1 Col): Vehicle & Hospital Readiness */}
        <div className="space-y-6">
          <AmbulanceTelemetry />
          <HospitalStatusCard />
        </div>
      </div>

      {/* Traffic Signals Cluster */}
      <div>
        <h3 className="font-heading-cyber font-bold text-white text-base mb-3 flex items-center space-x-2">
          <span>ROADSIDE INTERSECTION SIGNALS & HARDWARE GPIO PINOUTS</span>
        </h3>
        <SignalClusterView />
      </div>

      {/* Live Event Stream */}
      <div>
        <h3 className="font-heading-cyber font-bold text-white text-base mb-3 flex items-center space-x-2">
          <span>REAL-TIME AUDIT LOG & TELEMETRY STREAM</span>
        </h3>
        <LiveEventTable />
      </div>
    </div>
  );
};
