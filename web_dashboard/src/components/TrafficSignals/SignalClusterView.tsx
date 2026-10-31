import React from 'react';
import { TrafficCone, Radio, Cpu, ShieldCheck, Flame, AlertCircle } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const SignalClusterView: React.FC = () => {
  const { state } = useSimulation();

  return (
    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
      {state.intersections.map((node) => {
        const isPreempted = node.blueLed;
        return (
          <div
            key={node.id}
            className={`glass-panel p-5 rounded-xl border transition-all ${
              isPreempted
                ? 'border-cyan-500 shadow-[0_0_20px_rgba(0,240,255,0.25)] bg-cyan-950/20'
                : 'border-gray-800'
            }`}
          >
            {/* Header: Node Name and IP */}
            <div className="flex items-center justify-between pb-3 border-b border-gray-800">
              <div className="flex items-center space-x-2">
                <div
                  className={`w-7 h-7 rounded-lg flex items-center justify-center font-mono font-bold text-xs ${
                    isPreempted ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-400/40' : 'bg-gray-800 text-gray-300'
                  }`}
                >
                  #{node.id}
                </div>
                <div>
                  <h4 className="font-heading-cyber font-bold text-white text-sm">{node.name}</h4>
                  <p className="text-[11px] font-mono text-gray-500">
                    {node.ip}:{node.port}
                  </p>
                </div>
              </div>

              {/* Status Badge */}
              <span
                className={`text-[10px] font-mono px-2 py-0.5 rounded font-semibold ${
                  node.health === 'ONLINE'
                    ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30'
                    : 'bg-rose-500/20 text-rose-400 border border-rose-500/30 animate-pulse'
                }`}
              >
                {node.health}
              </span>
            </div>

            {/* Signal & Hardware LED Pods Layout */}
            <div className="py-4 flex items-center justify-around">
              {/* Traffic Signal Head (Red / Yellow / Green) */}
              <div className="flex flex-col items-center">
                <span className="text-[10px] font-mono text-gray-400 uppercase mb-1.5">Traffic Head</span>
                <div className="bg-[#050811] border border-gray-700/80 p-2.5 rounded-xl flex flex-col space-y-2.5 shadow-inner">
                  {/* Red Light */}
                  <div
                    className={`w-7 h-7 rounded-full border transition-all duration-300 ${
                      node.signalColor === 'RED'
                        ? 'bg-red-500 border-red-300 shadow-[0_0_12px_#EF4444]'
                        : 'bg-red-950/40 border-red-950/60'
                    }`}
                  />
                  {/* Yellow Light */}
                  <div
                    className={`w-7 h-7 rounded-full border transition-all duration-300 ${
                      node.signalColor === 'YELLOW'
                        ? 'bg-amber-400 border-amber-200 shadow-[0_0_12px_#F59E0B]'
                        : 'bg-amber-950/40 border-amber-950/60'
                    }`}
                  />
                  {/* Green Light */}
                  <div
                    className={`w-7 h-7 rounded-full border transition-all duration-300 ${
                      node.signalColor === 'GREEN'
                        ? 'bg-emerald-400 border-emerald-200 shadow-[0_0_14px_#10B981]'
                        : 'bg-emerald-950/40 border-emerald-950/60'
                    }`}
                  />
                </div>
              </div>

              {/* Physical GPIO LED Modules (Raspberry Pi Pinout) */}
              <div className="flex flex-col items-center">
                <span className="text-[10px] font-mono text-gray-400 uppercase mb-1.5">
                  RPi GPIO (330Ω)
                </span>
                <div className="bg-[#050811] border border-gray-700/80 p-3 rounded-xl flex flex-col space-y-3">
                  {/* Blue LED (Emergency Preemption) */}
                  <div className="flex items-center space-x-2">
                    <div
                      className={`w-6 h-6 rounded-full border transition-all duration-300 ${
                        node.blueLed
                          ? 'bg-cyan-400 border-cyan-200 shadow-[0_0_16px_#00F0FF] animate-pulse'
                          : 'bg-cyan-950/30 border-cyan-950/50'
                      }`}
                    />
                    <div className="text-[11px] font-mono">
                      <div className={node.blueLed ? 'text-cyan-300 font-bold' : 'text-gray-500'}>
                        BLUE: {node.blueLed ? 'ON' : 'OFF'}
                      </div>
                      <div className="text-[9px] text-gray-600">BCM 6 (Pin 31)</div>
                    </div>
                  </div>

                  {/* Red LED (Cross-Traffic Stop) */}
                  <div className="flex items-center space-x-2">
                    <div
                      className={`w-6 h-6 rounded-full border transition-all duration-300 ${
                        !node.hasRedHardware
                          ? 'bg-gray-800 border-gray-700 opacity-40'
                          : node.redLed
                          ? 'bg-red-500 border-red-200 shadow-[0_0_14px_#EF4444]'
                          : 'bg-red-950/30 border-red-950/50'
                      }`}
                    />
                    <div className="text-[11px] font-mono">
                      <div className={node.redLed ? 'text-red-400 font-bold' : 'text-gray-500'}>
                        {node.hasRedHardware ? `RED: ${node.redLed ? 'ON' : 'OFF'}` : 'RED: N/A'}
                      </div>
                      <div className="text-[9px] text-gray-600">
                        {node.hasRedHardware ? 'BCM 5 (Pin 29)' : 'Shortage Handled'}
                      </div>
                    </div>
                  </div>
                </div>
              </div>

              {/* Countdown / Status Dial */}
              <div className="flex flex-col items-center">
                <span className="text-[10px] font-mono text-gray-400 uppercase mb-1.5">Timer / Mode</span>
                <div className="w-16 h-16 rounded-xl bg-gray-900 border border-gray-800 flex flex-col items-center justify-center">
                  {isPreempted ? (
                    <>
                      <Flame className="w-6 h-6 text-cyan-400 animate-pulse" />
                      <span className="text-[9px] font-mono text-cyan-300 font-bold mt-1">HOLD</span>
                    </>
                  ) : (
                    <>
                      <span className="font-mono text-2xl font-bold text-white tracking-tight">
                        {node.countdown}
                      </span>
                      <span className="text-[9px] font-mono text-gray-500">sec</span>
                    </>
                  )}
                </div>
              </div>
            </div>

            {/* Bottom Telemetry Strip */}
            <div className="pt-3 border-t border-gray-800/80 flex items-center justify-between text-[11px]">
              <span className="text-gray-400">IR Sensor:</span>
              <span
                className={`font-mono ${
                  node.irSensorTriggered ? 'text-cyan-400 font-bold' : 'text-gray-600'
                }`}
              >
                {node.irSensorTriggered ? 'TRIGGERED (Vehicle Beam)' : 'STANDBY'}
              </span>
            </div>
            <div className="pt-1.5 flex items-center justify-between text-[11px]">
              <span className="text-gray-400">OLED Message:</span>
              <span className="font-mono text-gray-300 text-[10px] truncate max-w-[170px]">
                {node.oledStatus}
              </span>
            </div>
          </div>
        );
      })}
    </div>
  );
};
