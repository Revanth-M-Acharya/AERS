import React from 'react';
import { Cpu, RefreshCw, CheckCircle2, ShieldCheck, Layers, Radio } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { TwinOverview } from '../components/DigitalTwin/TwinOverview';

export const DigitalTwinPage: React.FC = () => {
  const { state, mode } = useSimulation();

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <Cpu className="w-5 h-5 text-cyan-400" />
          <span>CYBER-PHYSICAL DIGITAL TWIN CONTROL CENTER</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Real-Time Virtual Mirroring of QNX SDP 8.0 Raspberry Pi 4 Model B SBC Cluster
        </p>
      </div>

      <TwinOverview />

      {/* Synchronization Engine Specs */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
        <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center space-x-2 text-xs text-cyan-400 font-bold">
            <Radio className="w-4 h-4" />
            <span>TCP Socket Frame Mirroring</span>
          </div>
          <p className="text-xs text-gray-300">
            Every JSON control frame dispatched by the Coordinator is simultaneously mirrored into the
            in-browser digital twin at &lt; 1ms latency.
          </p>
          <div className="text-[11px] font-mono text-gray-500 pt-1">
            Checksum: CRC16-CCITT Validated
          </div>
        </div>

        <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center space-x-2 text-xs text-emerald-400 font-bold">
            <CheckCircle2 className="w-4 h-4" />
            <span>Hardware Shortage Virtualization</span>
          </div>
          <p className="text-xs text-gray-300">
            Intersection 3 hardware shortage (missing Red LED) is accurately represented in the twin:
            Red LED commands return safe virtual no-op acknowledgements.
          </p>
          <div className="text-[11px] font-mono text-gray-500 pt-1">
            Node 3: Pin 31 Blue Active / Red Virtualized
          </div>
        </div>

        <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-2">
          <div className="flex items-center space-x-2 text-xs text-amber-400 font-bold">
            <Layers className="w-4 h-4" />
            <span>Dual-Mode Interoperability</span>
          </div>
          <p className="text-xs text-gray-300">
            Seamless toggle between standalone desktop simulation and live hardware cluster telemetry
            without modifying any backend C++ code.
          </p>
          <div className="text-[11px] font-mono text-gray-500 pt-1">
            Active Mode: {mode} (100 Hz Sync)
          </div>
        </div>
      </div>
    </div>
  );
};
