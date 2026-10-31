import React from 'react';
import { Building2, Bed, Stethoscope, Clock, ShieldCheck, AlertCircle } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const HospitalStatusCard: React.FC = () => {
  const { state } = useSimulation();
  const hosp = state.hospital;
  const amb = state.ambulance;

  return (
    <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-4">
      {/* Header */}
      <div className="flex items-center justify-between pb-3 border-b border-gray-800">
        <div className="flex items-center space-x-3">
          <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
            <Building2 className="w-6 h-6" />
          </div>
          <div>
            <h3 className="font-heading-cyber font-bold text-white text-base">{hosp.name}</h3>
            <p className="text-xs text-gray-400 font-mono">Trauma Level 1 Facility</p>
          </div>
        </div>

        <span
          className={`text-[10px] font-mono px-2 py-0.5 rounded font-bold ${
            hosp.status === 'GREEN'
              ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30'
              : 'bg-rose-500/20 text-rose-400 border border-rose-500/30'
          }`}
        >
          {hosp.status === 'GREEN' ? 'RECEIVING READY' : 'DIVERT / CAPACITY'}
        </span>
      </div>

      {/* Hospital Capacities */}
      <div className="grid grid-cols-3 gap-3">
        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800 text-center">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <Bed className="w-3.5 h-3.5 text-cyan-400" />
            <span>Trauma Bays</span>
          </div>
          <div className="font-mono text-xl font-bold text-white">
            <span className="text-emerald-400">{hosp.traumaBaysAvailable}</span> / {hosp.totalTraumaBays}
          </div>
        </div>

        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800 text-center">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <Stethoscope className="w-3.5 h-3.5 text-indigo-400" />
            <span>OR Suite</span>
          </div>
          <div className="font-mono text-sm font-bold text-emerald-400 mt-1">
            {hosp.orReady ? 'STERILE / READY' : 'OCCUPIED'}
          </div>
        </div>

        <div className="bg-gray-900/80 p-3 rounded-lg border border-gray-800 text-center">
          <div className="flex items-center justify-center space-x-1.5 text-gray-400 text-xs mb-1">
            <Clock className="w-3.5 h-3.5 text-amber-400" />
            <span>ER Triage Wait</span>
          </div>
          <div className="font-mono text-xl font-bold text-white">
            {hosp.erWaitMinutes} <span className="text-xs text-gray-500">min</span>
          </div>
        </div>
      </div>

      {/* Incoming Ambulance Handshake Alert */}
      <div
        className={`p-3.5 rounded-lg border text-xs transition-all ${
          hosp.receivingTeamAlerted
            ? 'bg-cyan-950/40 border-cyan-500/40 text-cyan-200 shadow-[0_0_15px_rgba(0,240,255,0.15)]'
            : 'bg-gray-900/40 border-gray-800 text-gray-400'
        }`}
      >
        <div className="flex items-center justify-between font-semibold mb-1">
          <span className="flex items-center space-x-2">
            <ShieldCheck className="w-4 h-4 text-cyan-400" />
            <span>Trauma Team Alert Status:</span>
          </span>
          <span className="font-mono font-bold">
            {hosp.receivingTeamAlerted ? 'PRE-ALERTED (EN ROUTE)' : 'STANDBY'}
          </span>
        </div>
        <div className="text-[11px] font-mono text-gray-400">
          Inbound: {amb.id} | ETA: {amb.etaSeconds}s | Bed Allocated: Bay 4
        </div>
      </div>
    </div>
  );
};
