import React from 'react';
import { Building2, Bed, Stethoscope, Clock, ShieldCheck, Activity } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { HospitalStatusCard } from '../components/Hospital/HospitalStatusCard';

export const HospitalsPage: React.FC = () => {
  const { state } = useSimulation();

  const hospitals = [
    {
      id: 'HOSP-01',
      name: 'City General Hospital',
      type: 'Level-1 Trauma & Emergency Medical Center',
      baysAvailable: state.hospital.traumaBaysAvailable,
      totalBays: state.hospital.totalTraumaBays,
      orSuitesReady: 3,
      erWaitTime: state.hospital.erWaitMinutes,
      status: state.hospital.status,
      assigned: true,
    },
    {
      id: 'HOSP-02',
      name: 'Apollo Specialty Hospitals',
      type: 'Interventional Cardiology & Stroke Center',
      baysAvailable: 2,
      totalBays: 5,
      orSuitesReady: 2,
      erWaitTime: 8,
      status: 'GREEN',
      assigned: false,
    },
    {
      id: 'HOSP-03',
      name: "St. Martha's Medical Center",
      type: 'General Emergency & Trauma Division',
      baysAvailable: 5,
      totalBays: 8,
      orSuitesReady: 1,
      erWaitTime: 12,
      status: 'GREEN',
      assigned: false,
    },
  ];

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <Building2 className="w-5 h-5 text-cyan-400" />
          <span>HOSPITAL EMERGENCY DECK & TRAUMA CAPACITY MATRIX</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Automated Bed Allocation, Medical Team Pre-Alerting & ETA Handshake
        </p>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 space-y-4">
          {hospitals.map((h) => (
            <div
              key={h.id}
              className={`glass-panel p-5 rounded-xl border transition-all ${
                h.assigned
                  ? 'border-emerald-500 shadow-[0_0_15px_rgba(16,185,129,0.2)] bg-emerald-950/20'
                  : 'border-gray-800'
              }`}
            >
              <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-3 border-b border-gray-800 gap-2">
                <div className="flex items-center space-x-3">
                  <div className="w-9 h-9 rounded-lg bg-gray-800 flex items-center justify-center text-emerald-400">
                    <Building2 className="w-5 h-5" />
                  </div>
                  <div>
                    <h4 className="font-heading-cyber font-bold text-white text-base">{h.name}</h4>
                    <p className="text-[11px] font-mono text-gray-400">{h.type}</p>
                  </div>
                </div>

                <span
                  className={`text-[10px] font-mono px-2 py-0.5 rounded font-bold ${
                    h.assigned
                      ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 animate-pulse'
                      : 'bg-gray-800 text-gray-400'
                  }`}
                >
                  {h.assigned ? 'ASSIGNED RECEPTOR' : 'BACKUP FACILITY'}
                </span>
              </div>

              <div className="pt-3 grid grid-cols-3 gap-2 text-xs font-mono text-gray-400">
                <div className="flex items-center space-x-1.5">
                  <Bed className="w-3.5 h-3.5 text-cyan-400" />
                  <span>
                    Bays: <strong className="text-white">{h.baysAvailable}</strong> / {h.totalBays}
                  </span>
                </div>
                <div className="flex items-center space-x-1.5">
                  <Stethoscope className="w-3.5 h-3.5 text-indigo-400" />
                  <span>OR Suites: {h.orSuitesReady}</span>
                </div>
                <div className="flex items-center space-x-1.5">
                  <Clock className="w-3.5 h-3.5 text-amber-400" />
                  <span>Wait: {h.erWaitTime}m</span>
                </div>
              </div>
            </div>
          ))}
        </div>

        {/* Live Hospital Telemetry */}
        <div>
          <HospitalStatusCard />
        </div>
      </div>
    </div>
  );
};
