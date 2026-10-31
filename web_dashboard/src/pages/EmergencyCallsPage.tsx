import React from 'react';
import { PhoneCall, AlertTriangle, Flame, Clock, MapPin, User, CheckCircle2 } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { EmergencyPriority } from '../types';

export const EmergencyCallsPage: React.FC = () => {
  const { startMission, state } = useSimulation();

  const calls: Array<{
    id: string;
    caller: string;
    phone: string;
    location: string;
    time: string;
    nature: string;
    priority: EmergencyPriority;
    status: 'ACTIVE' | 'DISPATCHED' | 'STANDBY';
  }> = [
    {
      id: 'CALL-9041',
      caller: 'Priya Sharma (Bystander)',
      phone: '+91 98450 11234',
      location: '14th Main, Brigade Road Junction',
      time: '10:20:14',
      nature: 'Elderly male collapsed on sidewalk. Unresponsive, faint pulse.',
      priority: 'P1_CRITICAL',
      status: state.isRunning ? 'DISPATCHED' : 'ACTIVE',
    },
    {
      id: 'CALL-9040',
      caller: 'Security Patrol Desk',
      phone: '+91 80 2558 9911',
      location: 'Residency Cross Corporate Park',
      time: '10:14:02',
      nature: 'Industrial chemical inhalation in basement HVAC room.',
      priority: 'P2_URGENT',
      status: 'STANDBY',
    },
    {
      id: 'CALL-9039',
      caller: 'Traffic Sub-Inspector Kumar',
      phone: '+91 80 2294 2222',
      location: 'Richmond Circle Underpass',
      time: '10:02:45',
      nature: 'Two-wheeler skid during light rain. Fractured clavicle.',
      priority: 'P3_STANDARD',
      status: 'STANDBY',
    },
  ];

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800 flex items-center justify-between">
        <div>
          <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
            <PhoneCall className="w-5 h-5 text-cyan-400" />
            <span>EMERGENCY 108 / 911 CALL INTAKE & TRIAGE QUEUE</span>
          </h3>
          <p className="text-xs text-gray-400 font-mono mt-1">
            Real-time Tele-Triage & Direct Preemption Dispatch Integration
          </p>
        </div>
      </div>

      {/* Calls List */}
      <div className="space-y-4">
        {calls.map((call) => (
          <div
            key={call.id}
            className={`glass-panel p-5 rounded-xl border transition-all ${
              call.priority === 'P1_CRITICAL'
                ? 'border-rose-500/50 bg-rose-950/15 shadow-[0_0_15px_rgba(239,68,68,0.15)]'
                : 'border-gray-800'
            }`}
          >
            <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
              <div className="space-y-2">
                <div className="flex items-center space-x-3">
                  <span className="font-mono font-bold text-cyan-400 text-sm">{call.id}</span>
                  <span
                    className={`text-[10px] font-mono px-2 py-0.5 rounded font-bold ${
                      call.priority === 'P1_CRITICAL'
                        ? 'bg-rose-500/20 text-rose-300 border border-rose-500/40'
                        : 'bg-amber-500/20 text-amber-300 border border-amber-500/40'
                    }`}
                  >
                    {call.priority}
                  </span>
                  <span className="text-xs text-gray-400 font-mono flex items-center space-x-1">
                    <Clock className="w-3.5 h-3.5" />
                    <span>{call.time}</span>
                  </span>
                </div>

                <div className="text-sm font-semibold text-white">{call.nature}</div>

                <div className="flex flex-wrap items-center gap-4 text-xs font-mono text-gray-400">
                  <span className="flex items-center space-x-1 text-gray-300">
                    <User className="w-3.5 h-3.5 text-gray-500" />
                    <span>{call.caller} ({call.phone})</span>
                  </span>
                  <span className="flex items-center space-x-1 text-gray-300">
                    <MapPin className="w-3.5 h-3.5 text-gray-500" />
                    <span>{call.location}</span>
                  </span>
                </div>
              </div>

              {/* Action */}
              <div className="flex items-center space-x-3">
                <button
                  onClick={() => startMission(call.priority)}
                  disabled={state.isRunning}
                  className={`px-4 py-2 rounded-lg text-xs font-bold flex items-center space-x-2 transition ${
                    state.isRunning
                      ? 'bg-gray-800 text-gray-500 cursor-not-allowed border border-gray-700'
                      : 'btn-cyber-emergency'
                  }`}
                >
                  <Flame className="w-4 h-4" />
                  <span>{state.isRunning ? 'Unit Dispatched' : 'Preempt Corridor'}</span>
                </button>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
