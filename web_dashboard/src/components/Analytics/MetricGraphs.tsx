import React from 'react';
import {
  AreaChart,
  Area,
  LineChart,
  Line,
  BarChart,
  Bar,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import { Activity, Cpu, Radio, TrendingDown, Clock, Zap } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const MetricGraphs: React.FC = () => {
  const { state } = useSimulation();

  // Telemetry time series
  const performanceData = [
    { time: '10:00', packets: 98, cpu: 12, latency: 0.8 },
    { time: '10:05', packets: 110, cpu: 15, latency: 0.9 },
    { time: '10:10', packets: 145, cpu: 18, latency: 0.7 },
    { time: '10:15', packets: 210, cpu: 22, latency: 1.1 },
    { time: '10:20', packets: 180, cpu: 16, latency: 0.8 },
    { time: '10:25', packets: 195, cpu: 14, latency: 0.7 },
    { time: '10:30', packets: 225, cpu: 19, latency: 0.8 },
  ];

  // ETA Comparison (Before SMART AEGIS vs With SMART AEGIS)
  const etaComparisonData = [
    { corridor: 'MG Rd to Int 1', standardWait: 68, aegisPreempted: 24 },
    { corridor: 'Residency Cross', standardWait: 82, aegisPreempted: 31 },
    { corridor: 'Richmond Incline', standardWait: 94, aegisPreempted: 38 },
    { corridor: 'Total Transit', standardWait: 244, aegisPreempted: 93 },
  ];

  return (
    <div className="space-y-6">
      {/* KPI Cards Row */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div className="glass-panel p-4 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-2 text-xs text-gray-400 mb-1">
            <TrendingDown className="w-4 h-4 text-emerald-400" />
            <span>Avg ETA Reduction</span>
          </div>
          <div className="font-mono text-2xl font-bold text-emerald-400">
            {state.metrics.averageEtaReductionPercent}%
          </div>
          <p className="text-[10px] text-gray-500 mt-1">-151 seconds avg transit</p>
        </div>

        <div className="glass-panel p-4 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-2 text-xs text-gray-400 mb-1">
            <Radio className="w-4 h-4 text-cyan-400" />
            <span>Control Mesh RTT</span>
          </div>
          <div className="font-mono text-2xl font-bold text-white">
            {state.metrics.latencyMs} <span className="text-xs text-gray-400">ms</span>
          </div>
          <p className="text-[10px] text-gray-500 mt-1">Direct TCP Socket Mesh</p>
        </div>

        <div className="glass-panel p-4 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-2 text-xs text-gray-400 mb-1">
            <Zap className="w-4 h-4 text-amber-400" />
            <span>Throughput</span>
          </div>
          <div className="font-mono text-2xl font-bold text-amber-300">
            {state.metrics.packetsPerSec} <span className="text-xs text-gray-400">pkt/s</span>
          </div>
          <p className="text-[10px] text-gray-500 mt-1">Zero drop CRC16 frames</p>
        </div>

        <div className="glass-panel p-4 rounded-xl border border-gray-800">
          <div className="flex items-center space-x-2 text-xs text-gray-400 mb-1">
            <Cpu className="w-4 h-4 text-indigo-400" />
            <span>Coordinator CPU</span>
          </div>
          <div className="font-mono text-2xl font-bold text-white">
            {state.metrics.cpuUsagePercent}%
          </div>
          <p className="text-[10px] text-gray-500 mt-1">QNX SDP 8.0 Real-Time Thread</p>
        </div>
      </div>

      {/* Main Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Chart 1: ETA Comparison (Standard Traffic vs AEGIS Preemption) */}
        <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-3">
          <div className="flex items-center justify-between">
            <h4 className="font-heading-cyber font-bold text-white text-sm">
              TRANSIT TIME COMPARISON (SECONDS)
            </h4>
            <span className="text-[10px] font-mono text-emerald-400 bg-emerald-950/40 border border-emerald-500/30 px-2 py-0.5 rounded">
              61.8% Time Saved
            </span>
          </div>

          <div className="h-64 w-full">
            <ResponsiveContainer width="100%" height="100%">
              <BarChart data={etaComparisonData} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="#1F2937" />
                <XAxis dataKey="corridor" stroke="#6B7280" fontSize={11} />
                <YAxis stroke="#6B7280" fontSize={11} />
                <Tooltip
                  contentStyle={{ backgroundColor: '#111827', borderColor: '#374151', borderRadius: '8px' }}
                />
                <Bar dataKey="standardWait" fill="#EF4444" name="Standard Traffic (sec)" radius={[4, 4, 0, 0]} />
                <Bar dataKey="aegisPreempted" fill="#10B981" name="SMART AEGIS (sec)" radius={[4, 4, 0, 0]} />
              </BarChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Chart 2: Telemetry Mesh Throughput & Packet Rate */}
        <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-3">
          <div className="flex items-center justify-between">
            <h4 className="font-heading-cyber font-bold text-white text-sm">
              PACKETS / SEC & CPU LOAD TIMELINE
            </h4>
            <span className="text-[10px] font-mono text-cyan-400 bg-cyan-950/40 border border-cyan-500/30 px-2 py-0.5 rounded">
              High Resolution Telemetry
            </span>
          </div>

          <div className="h-64 w-full">
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart data={performanceData} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
                <defs>
                  <linearGradient id="packetGrad" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor="#00F0FF" stopOpacity={0.4} />
                    <stop offset="95%" stopColor="#00F0FF" stopOpacity={0} />
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="3 3" stroke="#1F2937" />
                <XAxis dataKey="time" stroke="#6B7280" fontSize={11} />
                <YAxis stroke="#6B7280" fontSize={11} />
                <Tooltip
                  contentStyle={{ backgroundColor: '#111827', borderColor: '#374151', borderRadius: '8px' }}
                />
                <Area type="monotone" dataKey="packets" stroke="#00F0FF" fillOpacity={1} fill="url(#packetGrad)" name="Packets / Sec" />
                <Line type="monotone" dataKey="cpu" stroke="#F59E0B" strokeWidth={2} name="CPU %" />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        </div>
      </div>
    </div>
  );
};
