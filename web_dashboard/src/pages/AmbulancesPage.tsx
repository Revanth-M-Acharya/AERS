import React from 'react';
import { Ambulance, Gauge, Fuel, ShieldCheck, Heart, Radio, Activity } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { AmbulanceTelemetry } from '../components/Ambulance/AmbulanceTelemetry';

export const AmbulancesPage: React.FC = () => {
  const { state } = useSimulation();

  const fleet = [
    {
      id: 'AMB-101',
      model: 'Force Motors Traveller ICU Grade-1',
      status: state.isRunning ? 'EN ROUTE' : 'STANDBY',
      fuel: 88,
      driver: 'Paramedic R. Nair (Badge #401)',
      vitalsActive: true,
      corridorActive: state.isRunning,
    },
    {
      id: 'AMB-102',
      model: 'Tata Winger Type-D Advanced Life Support',
      status: 'STATION ALPHA (STANDBY)',
      fuel: 95,
      driver: 'Paramedic S. Roy (Badge #412)',
      vitalsActive: false,
      corridorActive: false,
    },
    {
      id: 'AMB-103',
      model: 'Force Motors Traveller Trauma Suite',
      status: 'STATION BETA (STANDBY)',
      fuel: 91,
      driver: 'Paramedic M. Khan (Badge #428)',
      vitalsActive: false,
      corridorActive: false,
    },
  ];

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <Ambulance className="w-5 h-5 text-cyan-400" />
          <span>FLEET TELEMETRY & AMBULANCE DISPATCH DECK</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          GPS-Synchronized V2X Corridor Transmission Units
        </p>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 space-y-4">
          {fleet.map((veh) => (
            <div
              key={veh.id}
              className={`glass-panel p-5 rounded-xl border transition-all ${
                veh.corridorActive
                  ? 'border-cyan-500 shadow-[0_0_15px_rgba(0,240,255,0.2)] bg-cyan-950/20'
                  : 'border-gray-800'
              }`}
            >
              <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-3 border-b border-gray-800 gap-2">
                <div className="flex items-center space-x-3">
                  <div className="w-9 h-9 rounded-lg bg-gray-800 flex items-center justify-center text-cyan-400">
                    <Ambulance className="w-5 h-5" />
                  </div>
                  <div>
                    <h4 className="font-heading-cyber font-bold text-white text-base">{veh.id}</h4>
                    <p className="text-[11px] font-mono text-gray-400">{veh.model}</p>
                  </div>
                </div>

                <span
                  className={`text-[10px] font-mono px-2 py-0.5 rounded font-bold ${
                    veh.corridorActive
                      ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 animate-pulse'
                      : 'bg-gray-800 text-gray-400 border border-gray-700'
                  }`}
                >
                  {veh.status}
                </span>
              </div>

              <div className="pt-3 grid grid-cols-3 gap-2 text-xs font-mono text-gray-400">
                <div className="flex items-center space-x-1.5">
                  <Fuel className="w-3.5 h-3.5 text-amber-400" />
                  <span>Fuel: {veh.fuel}%</span>
                </div>
                <div className="flex items-center space-x-1.5">
                  <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
                  <span>ALS Equip: OK</span>
                </div>
                <div className="flex items-center space-x-1.5">
                  <Radio className="w-3.5 h-3.5 text-cyan-400" />
                  <span>Telemetry: 10Hz</span>
                </div>
              </div>
            </div>
          ))}
        </div>

        {/* Live Active Vehicle Card */}
        <div>
          <AmbulanceTelemetry />
        </div>
      </div>
    </div>
  );
};
