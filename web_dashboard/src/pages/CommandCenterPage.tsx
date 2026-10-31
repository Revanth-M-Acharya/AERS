import React from 'react';
import {
  ShieldAlert,
  Flame,
  Clock,
  MapPin,
  Building2,
  Ambulance,
  CheckCircle2,
  AlertTriangle,
} from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';

export const CommandCenterPage: React.FC = () => {
  const { state, startMission } = useSimulation();

  const activeMissions = [
    {
      id: 'MSN-2026-1031',
      priority: state.ambulance.priority,
      caller: 'City Medical Dispatch (E-911)',
      location: '14th Main, Brigade Road',
      assignedUnit: state.ambulance.id,
      hospital: state.hospital.name,
      status: state.isRunning ? 'EN ROUTE' : 'STANDBY',
      eta: state.ambulance.etaSeconds,
    },
  ];

  const historicalMissions = [
    {
      id: 'MSN-2026-1030',
      priority: 'P1_CRITICAL',
      location: 'MG Road Metro Station',
      assignedUnit: 'AMB-101',
      hospital: 'City General Hospital',
      duration: '4m 12s',
      result: 'COMPLETED (Transit -58%)',
    },
    {
      id: 'MSN-2026-1029',
      priority: 'P2_URGENT',
      location: 'Residency Cross Road',
      assignedUnit: 'AMB-102',
      hospital: 'Apollo Trauma Care',
      duration: '3m 45s',
      result: 'COMPLETED (Transit -64%)',
    },
    {
      id: 'MSN-2026-1028',
      priority: 'P1_CRITICAL',
      location: 'Richmond Circle Flyover',
      assignedUnit: 'AMB-101',
      hospital: 'City General Hospital',
      duration: '5m 02s',
      result: 'COMPLETED (Transit -61%)',
    },
  ];

  return (
    <div className="space-y-6">
      {/* Active Incident Header */}
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <div className="flex items-center justify-between pb-4 border-b border-gray-800">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 rounded-lg bg-cyan-500/10 border border-cyan-400/30 flex items-center justify-center text-cyan-400">
              <ShieldAlert className="w-6 h-6 animate-pulse" />
            </div>
            <div>
              <h3 className="font-heading-cyber font-bold text-white text-lg">
                INCIDENT COMMAND & EMERGENCY DISPATCH ORCHESTRATION
              </h3>
              <p className="text-xs text-gray-400 font-mono">
                Multi-Criteria Triage & Green Corridor Preemption Engine
              </p>
            </div>
          </div>

          <div className="flex items-center space-x-3">
            <button
              onClick={() => startMission('P1_CRITICAL')}
              disabled={state.isRunning}
              className={`flex items-center space-x-2 px-4 py-2 rounded-lg text-xs font-bold transition ${
                state.isRunning
                  ? 'bg-gray-800 text-gray-500 cursor-not-allowed'
                  : 'btn-cyber-emergency'
              }`}
            >
              <Flame className="w-4 h-4" />
              <span>Dispatch New P1 Emergency</span>
            </button>
          </div>
        </div>

        {/* Current Active Mission Card */}
        <div className="pt-4 grid grid-cols-1 md:grid-cols-4 gap-4 text-xs">
          <div className="bg-gray-900/60 p-3 rounded-lg border border-gray-800">
            <span className="text-gray-400 block mb-1">Incident ID:</span>
            <span className="font-mono text-cyan-300 font-bold">{activeMissions[0].id}</span>
          </div>
          <div className="bg-gray-900/60 p-3 rounded-lg border border-gray-800">
            <span className="text-gray-400 block mb-1">Target Hospital:</span>
            <span className="font-mono text-emerald-400 font-bold">{activeMissions[0].hospital}</span>
          </div>
          <div className="bg-gray-900/60 p-3 rounded-lg border border-gray-800">
            <span className="text-gray-400 block mb-1">Assigned Unit:</span>
            <span className="font-mono text-white font-bold">{activeMissions[0].assignedUnit}</span>
          </div>
          <div className="bg-gray-900/60 p-3 rounded-lg border border-gray-800">
            <span className="text-gray-400 block mb-1">Real-time ETA:</span>
            <span className="font-mono text-amber-300 font-bold">{activeMissions[0].eta}s</span>
          </div>
        </div>
      </div>

      {/* Corridor Decision Pipeline */}
      <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-4">
        <h4 className="font-heading-cyber font-bold text-white text-base">
          ACTIVE CORRIDOR DECISION PIPELINE (DIJKSTRA DYNAMIC PREEMPTION)
        </h4>

        <div className="grid grid-cols-1 md:grid-cols-5 gap-3">
          <div className="p-3 rounded-lg bg-gray-900 border border-gray-800 text-xs">
            <div className="text-[10px] text-gray-500 font-mono">STEP 1</div>
            <div className="font-bold text-gray-200 mt-1">Incident Ingestion</div>
            <div className="text-[11px] text-emerald-400 mt-0.5">Triage Verified</div>
          </div>

          <div className="p-3 rounded-lg bg-gray-900 border border-gray-800 text-xs">
            <div className="text-[10px] text-gray-500 font-mono">STEP 2</div>
            <div className="font-bold text-gray-200 mt-1">Hospital Scoring</div>
            <div className="text-[11px] text-emerald-400 mt-0.5">Bay 4 Locked</div>
          </div>

          <div className="p-3 rounded-lg bg-gray-900 border border-gray-800 text-xs">
            <div className="text-[10px] text-gray-500 font-mono">STEP 3</div>
            <div className="font-bold text-gray-200 mt-1">Route Dijkstra</div>
            <div className="text-[11px] text-cyan-400 mt-0.5">3 Intersections</div>
          </div>

          <div className="p-3 rounded-lg bg-gray-900 border border-gray-800 text-xs">
            <div className="text-[10px] text-gray-500 font-mono">STEP 4</div>
            <div className="font-bold text-gray-200 mt-1">Socket Preemption</div>
            <div className="text-[11px] text-cyan-400 mt-0.5">Blue LEDs Active</div>
          </div>

          <div className="p-3 rounded-lg bg-gray-900 border border-gray-800 text-xs">
            <div className="text-[10px] text-gray-500 font-mono">STEP 5</div>
            <div className="font-bold text-gray-200 mt-1">Safe Handover</div>
            <div className="text-[11px] text-gray-400 mt-0.5">ICU Admission</div>
          </div>
        </div>
      </div>

      {/* Historical Dispatch Records */}
      <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-4">
        <h4 className="font-heading-cyber font-bold text-white text-base">
          RECENT COMPLETED EMERGENCY CORRIDORS
        </h4>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="bg-gray-900 text-gray-400 uppercase text-[11px] border-b border-gray-800">
              <tr>
                <th className="py-2.5 px-3">Mission ID</th>
                <th className="py-2.5 px-3">Priority</th>
                <th className="py-2.5 px-3">Location</th>
                <th className="py-2.5 px-3">Unit</th>
                <th className="py-2.5 px-3">Hospital</th>
                <th className="py-2.5 px-3">Transit Time</th>
                <th className="py-2.5 px-3">Outcome</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-gray-800/60">
              {historicalMissions.map((m) => (
                <tr key={m.id} className="hover:bg-gray-900/40 transition">
                  <td className="py-2 px-3 text-cyan-300 font-bold">{m.id}</td>
                  <td className="py-2 px-3">
                    <span className="px-2 py-0.5 rounded text-[10px] bg-rose-500/20 text-rose-300 border border-rose-500/30">
                      {m.priority}
                    </span>
                  </td>
                  <td className="py-2 px-3 text-gray-300">{m.location}</td>
                  <td className="py-2 px-3 text-gray-200">{m.assignedUnit}</td>
                  <td className="py-2 px-3 text-emerald-400">{m.hospital}</td>
                  <td className="py-2 px-3 text-gray-400">{m.duration}</td>
                  <td className="py-2 px-3 text-emerald-400">{m.result}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
