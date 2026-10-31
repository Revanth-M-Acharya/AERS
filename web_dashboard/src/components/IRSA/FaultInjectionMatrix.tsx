import React from 'react';
import {
  AlertTriangle,
  Flame,
  CloudRain,
  EyeOff,
  VideoOff,
  ServerCrash,
  Car,
  Hammer,
  ShieldCheck,
  ShieldAlert,
  Waves,
  Ban,
  RotateCcw,
  Zap,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const FaultInjectionMatrix: React.FC = () => {
  const {
    state,
    toggleRoadBlocked,
    toggleAccident,
    toggleFlood,
    toggleConstruction,
    toggleBridgeClosed,
    toggleCameraFailure,
    toggleNodeFailure,
    toggleHeavyTraffic,
    togglePoliceOverride,
    setWeather,
    resetSimulation,
  } = useSimulation();

  // Determine active states
  const isRain = state.weather.mode === 'RAIN';
  const isFog = state.weather.mode === 'FOG';

  const faultButtons = [
    {
      id: 'road-closed',
      label: 'ROAD CLOSED',
      description: 'Block ROAD-01 (MG Road Expressway)',
      active: state.roadBlocked,
      onClick: () => toggleRoadBlocked(),
      icon: Ban,
      color: 'rose',
    },
    {
      id: 'accident',
      label: 'VEHICLE ACCIDENT',
      description: 'Crash on ROAD-02 (Residency Link)',
      active: state.accidentOnRoute,
      onClick: () => toggleAccident(),
      icon: Flame,
      color: 'rose',
    },
    {
      id: 'flood',
      label: 'FLASH FLOODING',
      description: 'Waterlogging on Victoria South cut',
      active: state.floodActive,
      onClick: () => toggleFlood(),
      icon: Waves,
      color: 'blue',
    },
    {
      id: 'heavy-rain',
      label: 'HEAVY RAINFALL',
      description: 'Precipitation friction & speed cap',
      active: isRain,
      onClick: () => setWeather(isRain ? 'CLEAR' : 'RAIN'),
      icon: CloudRain,
      color: 'blue',
    },
    {
      id: 'dense-fog',
      label: 'DENSE FOG',
      description: 'Low visibility mode (40% optical)',
      active: isFog,
      onClick: () => setWeather(isFog ? 'CLEAR' : 'FOG'),
      icon: EyeOff,
      color: 'amber',
    },
    {
      id: 'cam-failure',
      label: 'CAMERA FAILURE',
      description: 'CAM-02 drops offline (0 FPS)',
      active: state.cameraFailure,
      onClick: () => toggleCameraFailure(),
      icon: VideoOff,
      color: 'amber',
    },
    {
      id: 'node-failure',
      label: 'NODE 2 FAILURE',
      description: 'Pi 2 power drop -> Auto Reroute',
      active: state.intersectionFailed,
      onClick: () => toggleNodeFailure(2),
      icon: ServerCrash,
      color: 'rose',
    },
    {
      id: 'traffic-surge',
      label: 'TRAFFIC GRIDLOCK',
      description: 'Arterial density surges to 92%',
      active: state.heavyTraffic,
      onClick: () => toggleHeavyTraffic(),
      icon: Car,
      color: 'amber',
    },
    {
      id: 'construction',
      label: 'ROADWORKS / UTILITY',
      description: 'Lane constriction on ROAD-03',
      active: state.constructionActive,
      onClick: () => toggleConstruction(),
      icon: Hammer,
      color: 'amber',
    },
    {
      id: 'police-block',
      label: 'POLICE OVERRIDE',
      description: 'Priority escort cruisers deployed',
      active: state.policeEscort.active,
      onClick: () => togglePoliceOverride(),
      icon: ShieldAlert,
      color: 'cyan',
    },
    {
      id: 'bridge-closed',
      label: 'BRIDGE CLOSED',
      description: 'Halasuru Overpass Bridge shutdown',
      active: state.bridgeClosed,
      onClick: () => toggleBridgeClosed(),
      icon: AlertTriangle,
      color: 'rose',
    },
  ];

  const activeCount = faultButtons.filter((f) => f.active).length;

  return (
    <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-5 shadow-2xl flex flex-col space-y-4">
      {/* Header */}
      <div className="flex items-center justify-between border-b border-gray-800 pb-3">
        <div className="flex items-center space-x-2.5">
          <div className="p-2 rounded-lg bg-rose-500/10 border border-rose-500/20 text-rose-400">
            <AlertTriangle size={18} />
          </div>
          <div>
            <h3 className="font-bold text-gray-100 text-sm tracking-wide">
              FAULT INJECTION & CHAOS ENGINEERING MATRIX
            </h3>
            <p className="text-xs text-gray-400">
              Inject real-world disruptions to demonstrate QNX autonomous failover, Dijkstra rerouting & IRSA resilience
            </p>
          </div>
        </div>

        <div className="flex items-center space-x-3">
          <span className="text-xs font-mono text-gray-400">
            ACTIVE FAULTS:{' '}
            <span className={`font-bold ${activeCount > 0 ? 'text-rose-400' : 'text-emerald-400'}`}>
              {activeCount} / 11
            </span>
          </span>
          {activeCount > 0 && (
            <button
              onClick={resetSimulation}
              className="px-2.5 py-1 text-xs font-mono font-bold rounded bg-gray-800 hover:bg-gray-700 text-gray-300 border border-gray-700 flex items-center space-x-1 transition"
            >
              <RotateCcw size={12} className="mr-1" /> CLEAR ALL
            </button>
          )}
        </div>
      </div>

      {/* 11 Scenario Buttons in Responsive Grid */}
      <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-6 gap-3">
        {faultButtons.map((btn) => {
          const Icon = btn.icon;
          return (
            <button
              key={btn.id}
              onClick={btn.onClick}
              className={`p-3 rounded-xl border text-left flex flex-col justify-between transition-all duration-200 group relative overflow-hidden ${
                btn.active
                  ? btn.color === 'rose'
                    ? 'bg-rose-950/40 border-rose-500 shadow-rose-950/50 shadow-md text-rose-200'
                    : btn.color === 'blue'
                    ? 'bg-blue-950/40 border-blue-500 shadow-blue-950/50 shadow-md text-blue-200'
                    : btn.color === 'cyan'
                    ? 'bg-cyan-950/40 border-cyan-500 shadow-cyan-950/50 shadow-md text-cyan-200'
                    : 'bg-amber-950/40 border-amber-500 shadow-amber-950/50 shadow-md text-amber-200'
                  : 'bg-[#0E1526] border-gray-800/80 hover:border-gray-700 text-gray-400 hover:text-gray-200'
              }`}
            >
              <div className="flex items-center justify-between w-full mb-2">
                <Icon
                  size={18}
                  className={`transition ${
                    btn.active
                      ? btn.color === 'rose'
                        ? 'text-rose-400'
                        : btn.color === 'blue'
                        ? 'text-blue-400'
                        : btn.color === 'cyan'
                        ? 'text-cyan-400'
                        : 'text-amber-400'
                      : 'text-gray-500 group-hover:text-gray-400'
                  }`}
                />
                <span
                  className={`text-[9px] font-mono px-1.5 py-0.5 rounded font-bold ${
                    btn.active ? 'bg-rose-500/20 text-rose-300' : 'bg-gray-800 text-gray-500'
                  }`}
                >
                  {btn.active ? 'TRIGGERED' : 'READY'}
                </span>
              </div>

              <div>
                <span className="font-bold text-xs block leading-tight text-gray-200 group-hover:text-white">
                  {btn.label}
                </span>
                <span className="text-[10px] text-gray-500 block mt-1 line-clamp-2 leading-tight">
                  {btn.description}
                </span>
              </div>
            </button>
          );
        })}
      </div>
    </div>
  );
};
