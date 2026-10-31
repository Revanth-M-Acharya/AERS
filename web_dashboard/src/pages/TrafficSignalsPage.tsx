import React from 'react';
import { TrafficCone, Cpu, Radio, ShieldCheck, Layers, AlertCircle } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { SignalClusterView } from '../components/TrafficSignals/SignalClusterView';

export const TrafficSignalsPage: React.FC = () => {
  const { state } = useSimulation();

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <TrafficCone className="w-5 h-5 text-cyan-400" />
          <span>ROADSIDE INTERSECTIONS & QNX GPIO HARDWARE REGISTER MESH</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Zero-Relay Direct Control (330Ω Series Resistors) via BCM2711 Memory Registers (0xFE200000)
        </p>
      </div>

      {/* Signals Cluster Cards */}
      <SignalClusterView />

      {/* Hardware Engineering Specification Table */}
      <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-4">
        <h4 className="font-heading-cyber font-bold text-white text-base">
          PHYSICAL GPIO PINOUTS & HARDWARE SHORTAGE SPECIFICATION
        </h4>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="bg-gray-900 text-gray-400 uppercase text-[11px] border-b border-gray-800">
              <tr>
                <th className="py-2.5 px-3">Node</th>
                <th className="py-2.5 px-3">Location</th>
                <th className="py-2.5 px-3">IP : Port</th>
                <th className="py-2.5 px-3">Blue LED (Emergency)</th>
                <th className="py-2.5 px-3">Red LED (Cross-Traffic)</th>
                <th className="py-2.5 px-3">Hardware Status</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-gray-800/60">
              <tr className="hover:bg-gray-900/40 transition">
                <td className="py-2.5 px-3 font-bold text-white">Pi 2 (Node 1)</td>
                <td className="py-2.5 px-3 text-gray-300">MG Road - Brigade Junction</td>
                <td className="py-2.5 px-3 text-cyan-300">10.204.179.245 : 5001</td>
                <td className="py-2.5 px-3 text-cyan-400 font-bold">BCM 6 (Pin 31) - 330Ω</td>
                <td className="py-2.5 px-3 text-rose-400 font-bold">BCM 5 (Pin 29) - 330Ω</td>
                <td className="py-2.5 px-3 text-emerald-400">DUAL LED NOMINAL</td>
              </tr>
              <tr className="hover:bg-gray-900/40 transition">
                <td className="py-2.5 px-3 font-bold text-white">Pi 3 (Node 2)</td>
                <td className="py-2.5 px-3 text-gray-300">Residency Road Cross</td>
                <td className="py-2.5 px-3 text-cyan-300">10.204.179.130 : 5002</td>
                <td className="py-2.5 px-3 text-cyan-400 font-bold">BCM 6 (Pin 31) - 330Ω</td>
                <td className="py-2.5 px-3 text-rose-400 font-bold">BCM 5 (Pin 29) - 330Ω</td>
                <td className="py-2.5 px-3 text-emerald-400">DUAL LED NOMINAL</td>
              </tr>
              <tr className="hover:bg-gray-900/40 transition">
                <td className="py-2.5 px-3 font-bold text-white">Pi 4 (Node 3)</td>
                <td className="py-2.5 px-3 text-gray-300">Richmond Circle</td>
                <td className="py-2.5 px-3 text-cyan-300">10.204.179.144 : 5003</td>
                <td className="py-2.5 px-3 text-cyan-400 font-bold">BCM 6 (Pin 31) - 330Ω</td>
                <td className="py-2.5 px-3 text-gray-500 italic">NONE (Shortage)</td>
                <td className="py-2.5 px-3 text-amber-300 font-bold">
                  SHORTAGE (Blue Only; Red No-Op)
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};
