/**
 * @file ActionPlanCard.tsx
 * @brief Autonomous Action Plan & Hospital Recommendation Card.
 * Displays calculated preemption strategy, hospital routing, and dispatch controls.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import { IRSAResult } from '../../types';
import {
  Hospital,
  ShieldAlert,
  Flame,
  Radio,
  Clock,
  Navigation,
  UserCheck,
  PackageCheck,
  CheckCircle2,
} from 'lucide-react';

interface ActionPlanCardProps {
  irsa: IRSAResult;
  onDispatch: () => void;
  isDispatching?: boolean;
}

export const ActionPlanCard: React.FC<ActionPlanCardProps> = ({
  irsa,
  onDispatch,
  isDispatching = false,
}) => {
  const { recommendedHospital, automaticActions, priorityClass, priorityColor } = irsa;

  return (
    <div className="bg-[#0F172A] border border-slate-800 rounded-xl p-5 space-y-5 shadow-xl">
      {/* Header */}
      <div className="flex items-center justify-between pb-3 border-b border-slate-800">
        <div className="flex items-center space-x-2">
          <div className="p-2 rounded-lg bg-cyan-500/10 text-cyan-400 border border-cyan-500/30">
            <ShieldAlert size={18} />
          </div>
          <div>
            <h3 className="text-sm font-bold text-white uppercase tracking-wider font-mono">
              Autonomous Action Plan
            </h3>
            <p className="text-[11px] text-gray-400 font-mono">
              Signal Preemption Level: <span className="text-cyan-400 font-bold">{automaticActions.signalOverrideLevel}</span>
            </p>
          </div>
        </div>

        <span
          className="px-2.5 py-1 rounded text-[11px] font-mono font-bold uppercase tracking-wider border"
          style={{
            backgroundColor: `${priorityColor}15`,
            color: priorityColor,
            borderColor: `${priorityColor}40`,
          }}
        >
          {automaticActions.hospitalAlert.replace(/_/g, ' ')}
        </span>
      </div>

      {/* Hospital Recommendation */}
      <div className="bg-slate-900/80 border border-slate-700/60 rounded-lg p-4 space-y-3">
        <div className="flex items-start justify-between">
          <div className="flex items-center space-x-2.5">
            <Hospital className="text-rose-400 flex-shrink-0" size={20} />
            <div>
              <div className="text-sm font-bold text-white">{recommendedHospital.name}</div>
              <div className="text-xs text-rose-300 font-mono">{recommendedHospital.specialty}</div>
            </div>
          </div>
          <div className="text-right">
            <span className="text-xs px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-mono font-bold border border-emerald-500/30">
              {recommendedHospital.suitabilityScore}% MATCH
            </span>
            <div className="text-[11px] text-gray-400 font-mono mt-1">{recommendedHospital.distanceKm} km away</div>
          </div>
        </div>

        <p className="text-xs text-gray-300 italic bg-slate-950/60 p-2.5 rounded border border-slate-800">
          "{recommendedHospital.rationale}"
        </p>

        <div className="grid grid-cols-3 gap-2 pt-1 font-mono text-xs">
          <div className="bg-slate-950 p-2 rounded border border-slate-800 text-center">
            <div className="text-[10px] text-gray-400 uppercase">Trauma Bays</div>
            <div className="text-sm font-bold text-emerald-400">{recommendedHospital.traumaBaysAvailable} Free</div>
          </div>
          <div className="bg-slate-950 p-2 rounded border border-slate-800 text-center">
            <div className="text-[10px] text-gray-400 uppercase">ICU Beds</div>
            <div className="text-sm font-bold text-cyan-400">{recommendedHospital.icuBedsAvailable} Ready</div>
          </div>
          <div className="bg-slate-950 p-2 rounded border border-slate-800 text-center">
            <div className="text-[10px] text-gray-400 uppercase">ER Wait</div>
            <div className="text-sm font-bold text-white">{recommendedHospital.erWaitMinutes} min</div>
          </div>
        </div>
      </div>

      {/* Corridor & Preemption Specs */}
      <div className="grid grid-cols-2 gap-3 text-xs font-mono">
        <div className="bg-slate-900/60 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="text-gray-400 flex items-center space-x-1.5">
            <Navigation size={13} className="text-cyan-400" />
            <span>Corridor Scope</span>
          </div>
          <div className="text-sm font-bold text-white">
            {automaticActions.greenCorridorLength} Intersections
          </div>
          <div className="text-[10px] text-gray-400 leading-tight">
            {automaticActions.corridorStrategy}
          </div>
        </div>

        <div className="bg-slate-900/60 p-3 rounded-lg border border-slate-800 space-y-1">
          <div className="text-gray-400 flex items-center space-x-1.5">
            <Clock size={13} className="text-amber-400" />
            <span>Target Transit ETA</span>
          </div>
          <div className="text-sm font-bold text-amber-400">
            {automaticActions.etaTargetMinutes} Minutes
          </div>
          <div className="text-[10px] text-gray-400">
            Max Waiting: {automaticActions.maxWaitingSec}s | Hold: {automaticActions.maxSignalHoldSec}s
          </div>
        </div>
      </div>

      {/* Police & Broadcast Badges */}
      <div className="grid grid-cols-2 gap-3 text-xs font-mono">
        <div className="bg-slate-900/60 p-3 rounded-lg border border-slate-800 flex items-center justify-between">
          <span className="text-gray-400">Police Escort</span>
          <span
            className={`font-bold px-2 py-0.5 rounded text-[10px] ${
              automaticActions.policeEscort === 'MANDATORY'
                ? 'bg-rose-500/20 text-rose-300 border border-rose-500/40'
                : 'bg-slate-800 text-gray-300'
            }`}
          >
            {automaticActions.policeEscort}
          </span>
        </div>

        <div className="bg-slate-900/60 p-3 rounded-lg border border-slate-800 flex items-center justify-between">
          <span className="text-gray-400">Traffic V2X Alert</span>
          <span className="font-bold text-cyan-400 flex items-center space-x-1">
            <Radio size={12} className="animate-pulse" />
            <span>{automaticActions.trafficBroadcast ? 'BROADCASTING' : 'OFF'}</span>
          </span>
        </div>
      </div>

      {/* Alerted Medical Specialists */}
      <div className="space-y-1.5 text-xs font-mono">
        <div className="text-gray-400 flex items-center space-x-1.5">
          <UserCheck size={14} className="text-emerald-400" />
          <span>Automated Doctor Pre-Alerts:</span>
        </div>
        <div className="flex flex-wrap gap-1.5">
          {automaticActions.doctorSpecialtiesAlerted.map((spec) => (
            <span
              key={spec}
              className="px-2 py-1 bg-emerald-950/40 text-emerald-300 border border-emerald-500/30 rounded text-[11px]"
            >
              ✓ {spec}
            </span>
          ))}
        </div>
      </div>

      {/* Required Equipment Badges */}
      {automaticActions.requiredResources.length > 0 && (
        <div className="space-y-1.5 text-xs font-mono">
          <div className="text-gray-400 flex items-center space-x-1.5">
            <PackageCheck size={14} className="text-cyan-400" />
            <span>Required Staged Resources:</span>
          </div>
          <div className="flex flex-wrap gap-1.5">
            {automaticActions.requiredResources.map((res) => (
              <span
                key={res}
                className="px-2 py-0.5 bg-blue-950/40 text-blue-300 border border-blue-500/30 rounded text-[10px]"
              >
                ● {res}
              </span>
            ))}
          </div>
        </div>
      )}

      {/* Master Action Trigger Button */}
      <button
        onClick={onDispatch}
        disabled={isDispatching}
        className={`w-full py-3.5 px-4 rounded-xl font-mono font-bold text-sm tracking-wider uppercase transition-all shadow-lg flex items-center justify-center space-x-2 ${
          priorityClass === 'P1_CRITICAL'
            ? 'bg-gradient-to-r from-red-600 via-rose-600 to-red-700 hover:from-red-500 hover:to-rose-600 text-white shadow-[0_0_20px_rgba(239,68,68,0.5)] animate-pulse'
            : priorityClass === 'P2_HIGH'
            ? 'bg-gradient-to-r from-orange-600 to-amber-600 hover:from-orange-500 hover:to-amber-500 text-white shadow-[0_0_15px_rgba(249,115,22,0.4)]'
            : 'bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white shadow-[0_0_15px_rgba(6,182,212,0.4)]'
        }`}
      >
        <Flame size={18} />
        <span>Engage Green Corridor & Dispatch Mission</span>
      </button>
    </div>
  );
};
