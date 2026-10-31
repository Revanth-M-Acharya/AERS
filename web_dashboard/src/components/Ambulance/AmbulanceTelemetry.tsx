import React from 'react';
import {
  Ambulance,
  Gauge,
  Clock,
  MapPin,
  Heart,
  Droplet,
  Activity,
  Flame,
  AlertTriangle,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const AmbulanceTelemetry: React.FC = () => {
  const { state } = useSimulation();
  const amb = state.ambulance;

  return (
    <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-5">
      {/* Header */}
      <div className="flex items-center justify-between pb-3 border-b border-gray-800">
        <div className="flex items-center space-x-3">
          <div className="w-10 h-10 rounded-lg bg-rose-500/10 border border-rose-500/30 flex items-center justify-center text-rose-400">
            <Ambulance className="w-6 h-6 animate-pulse" />
          </div>
          <div>
            <div className="flex items-center space-x-2">
              <h3 className="font-heading-cyber font-bold text-white text-base">{amb.id}</h3>
              <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-rose-500/20 text-rose-300 border border-rose-500/30 font-semibold">
                {amb.priority}
              </span>
            </div>
            <p className="text-xs text-gray-400 font-mono">{amb.callSign}</p>
          </div>
        </div>

        <div className="text-right">
          <div className="text-[11px] text-gray-400 font-mono">STATUS</div>
          <span
            className={`font-mono text-xs font-bold ${
              amb.isBreakdown
                ? 'text-purple-400'
                : amb.isActive
                ? 'text-cyan-400 animate-pulse'
                : 'text-gray-500'
            }`}
          >
            {amb.isBreakdown ? 'BREAKDOWN' : amb.isActive ? 'ACTIVE CORRIDOR' : 'STANDBY'}
          </span>
        </div>
      </div>

      {/* Speed, ETA & Road Coordinates */}
      <div className="grid grid-cols-3 gap-3 text-center">
        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <Gauge className="w-3.5 h-3.5 text-cyan-400" />
            <span>Velocity</span>
          </div>
          <div className="font-mono text-xl font-bold text-white">
            {amb.speedKmh} <span className="text-xs text-gray-500">km/h</span>
          </div>
        </div>

        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <Clock className="w-3.5 h-3.5 text-emerald-400" />
            <span>ETA</span>
          </div>
          <div className="font-mono text-xl font-bold text-emerald-400">
            {amb.etaSeconds} <span className="text-xs text-gray-500">sec</span>
          </div>
        </div>

        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <MapPin className="w-3.5 h-3.5 text-amber-400" />
            <span>Progress</span>
          </div>
          <div className="font-mono text-xl font-bold text-cyan-300">
            {amb.positionPercent}%
          </div>
        </div>
      </div>

      {/* Progression Bar */}
      <div>
        <div className="flex justify-between text-xs text-gray-400 font-mono mb-1.5">
          <span>Corridor Traversal</span>
          <span>{amb.currentRoad}</span>
        </div>
        <div className="w-full h-2.5 bg-gray-900 rounded-full overflow-hidden border border-gray-800">
          <div
            className="h-full bg-gradient-to-r from-blue-500 via-cyan-400 to-emerald-400 transition-all duration-300 shadow-[0_0_10px_rgba(0,240,255,0.5)]"
            style={{ width: `${amb.positionPercent}%` }}
          />
        </div>
      </div>

      {/* Patient Telemetry Stream */}
      <div className="bg-gray-900/60 p-3.5 rounded-lg border border-gray-800 space-y-2">
        <div className="flex items-center justify-between text-xs font-semibold text-gray-300">
          <div className="flex items-center space-x-1.5">
            <Activity className="w-3.5 h-3.5 text-rose-400" />
            <span>Patient Triage Vitals</span>
          </div>
          <span className="text-[10px] font-mono text-cyan-400">{amb.patientCondition}</span>
        </div>

        <div className="grid grid-cols-3 gap-2 text-xs font-mono pt-1">
          <div className="flex items-center space-x-1.5 text-gray-400">
            <Heart className="w-3.5 h-3.5 text-rose-500" />
            <span>{amb.heartRateBpm} BPM</span>
          </div>
          <div className="flex items-center space-x-1.5 text-gray-400">
            <Droplet className="w-3.5 h-3.5 text-blue-400" />
            <span>{amb.spO2Percent}% SpO2</span>
          </div>
          <div className="flex items-center space-x-1.5 text-gray-400">
            <Activity className="w-3.5 h-3.5 text-amber-400" />
            <span>{amb.bloodPressure}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
