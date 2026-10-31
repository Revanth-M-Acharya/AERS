/**
 * @file PrioritySchedulerPage.tsx
 * @brief Intelligent Priority Scheduler & Clinical Triage Operations Page.
 * Allows judges and operators to simulate emergency patients, execute IRSA scoring,
 * view recommended hospital routing, and trigger preemptive green wave corridors.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useState, useMemo } from 'react';
import { PatientCase } from '../types';
import { DEFAULT_PATIENT_CASE, calculateIRSA } from '../services/irsaEngine';
import { PatientInputForm } from '../components/PriorityScheduler/PatientInputForm';
import { CircularPriorityMeter } from '../components/PriorityScheduler/CircularPriorityMeter';
import { SeverityBreakdownChart } from '../components/PriorityScheduler/SeverityBreakdownChart';
import { ActionPlanCard } from '../components/PriorityScheduler/ActionPlanCard';
import { useSimulation } from '../hooks/useSimulation';
import { SlidersHorizontal, Activity, Cpu, CheckCircle } from 'lucide-react';

export const PrioritySchedulerPage: React.FC = () => {
  const [patient, setPatient] = useState<PatientCase>(DEFAULT_PATIENT_CASE);
  const [isDispatched, setIsDispatched] = useState<boolean>(false);
  const { startMission, logEvent } = useSimulation();

  // Dynamic IRSA mathematical calculation on any patient form change
  const irsa = useMemo(() => {
    return calculateIRSA(patient);
  }, [patient]);

  const handleReset = () => {
    setPatient(DEFAULT_PATIENT_CASE);
    setIsDispatched(false);
  };

  const handleDispatch = () => {
    // Start active simulation mission with the calculated priority
    startMission(irsa.priorityClass);
    setIsDispatched(true);

    logEvent({
      source: 'PRIORITY_SCHEDULER',
      level: 'EMERGENCY',
      message: `DISPATCHED: ${patient.patientName} (${irsa.priorityLabel}) -> Routing to ${irsa.recommendedHospital.name}`,
      details: `Preemption Level: ${irsa.automaticActions.signalOverrideLevel} | Corridor: ${irsa.automaticActions.greenCorridorLength} nodes`,
    });

    setTimeout(() => {
      setIsDispatched(false);
    }, 4000);
  };

  return (
    <div className="space-y-6">
      {/* Top Banner */}
      <div className="flex flex-col md:flex-row md:items-center justify-between gap-4 pb-4 border-b border-gray-800">
        <div>
          <div className="flex items-center space-x-2">
            <span className="p-2 rounded-lg bg-cyan-500/10 text-cyan-400 border border-cyan-500/30">
              <SlidersHorizontal size={20} />
            </span>
            <div>
              <h1 className="text-xl font-black text-white font-mono tracking-wider uppercase">
                Intelligent Priority Scheduler
              </h1>
              <p className="text-xs text-gray-400 font-mono">
                IRSA Engine (Intelligent Risk & Severity Assessment) • Dynamic Preemption Calculator
              </p>
            </div>
          </div>
        </div>

        <div className="flex items-center space-x-3 text-xs font-mono">
          <div className="flex items-center space-x-2 px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800">
            <Cpu size={14} className="text-cyan-400" />
            <span className="text-gray-400">IRSA Engine:</span>
            <span className="text-emerald-400 font-bold">ONLINE (v2.5)</span>
          </div>

          <div className="flex items-center space-x-2 px-3 py-1.5 rounded-lg bg-slate-900 border border-slate-800">
            <Activity size={14} className="text-rose-400 animate-pulse" />
            <span className="text-gray-400">Triage Grade:</span>
            <span className="font-bold font-mono" style={{ color: irsa.priorityColor }}>
              {irsa.priorityLabel}
            </span>
          </div>
        </div>
      </div>

      {isDispatched && (
        <div className="p-3 bg-emerald-950/80 border border-emerald-500/60 rounded-xl flex items-center justify-between text-emerald-200 text-xs font-mono shadow-[0_0_20px_rgba(16,185,129,0.3)] animate-bounce">
          <div className="flex items-center space-x-2.5">
            <CheckCircle size={18} className="text-emerald-400" />
            <span>
              EMERGENCY MISSION ACTIVE: Corridor locks staged across {irsa.automaticActions.greenCorridorLength} intersections. Ambulance en route to {irsa.recommendedHospital.name}.
            </span>
          </div>
        </div>
      )}

      {/* Main Dual-Column Content */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Column: Comprehensive Patient Input Form (7 cols) */}
        <div className="lg:col-span-7">
          <PatientInputForm patient={patient} onChange={setPatient} onReset={handleReset} />
        </div>

        {/* Right Column: IRSA Meters, Breakdown, Action Plan (5 cols) */}
        <div className="lg:col-span-5 space-y-6">
          {/* Priority Meter Card */}
          <div className="bg-[#0F172A] border border-slate-800 rounded-xl p-5 shadow-xl">
            <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider mb-4 pb-2 border-b border-slate-800 flex items-center justify-between">
              <span>Automated Priority Score</span>
              <span className="text-[10px] text-gray-500 font-mono">Weighted Multi-Criteria</span>
            </div>

            <CircularPriorityMeter
              score={irsa.priorityScore}
              priorityClass={irsa.priorityClass}
              priorityLabel={irsa.priorityLabel}
              color={irsa.priorityColor}
            />
          </div>

          {/* Severity Breakdown Card */}
          <div className="bg-[#0F172A] border border-slate-800 rounded-xl p-5 shadow-xl space-y-4">
            <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider pb-2 border-b border-slate-800 flex items-center justify-between">
              <span>Severity Breakdown</span>
              <span className="text-[10px] text-cyan-400 font-mono">Real-Time IRSA</span>
            </div>

            <SeverityBreakdownChart breakdown={irsa.severityBreakdown} />
          </div>

          {/* Action Plan & Hospital Recommendation Card */}
          <ActionPlanCard irsa={irsa} onDispatch={handleDispatch} isDispatching={isDispatched} />
        </div>
      </div>
    </div>
  );
};
