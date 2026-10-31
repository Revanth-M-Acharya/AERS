import React, { useState } from 'react';
import {
  Activity,
  Video,
  Eye,
  AlertTriangle,
  Clock,
  TrendingUp,
  ShieldCheck,
  Search,
  Filter,
  ArrowRight,
  Wifi,
  Radio,
  Sparkles,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';
import { RoadData } from '../../types';

export const IRSAPanel: React.FC = () => {
  const { state, toggleRoadBlocked, toggleAccident, toggleFlood, toggleCameraFailure } = useSimulation();
  const [searchTerm, setSearchTerm] = useState('');
  const [filterDensity, setFilterDensity] = useState<'ALL' | 'GREEN' | 'YELLOW' | 'ORANGE' | 'RED'>('ALL');

  // Filter roads
  const filteredRoads = state.roads.filter((r) => {
    const matchesSearch =
      r.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
      r.id.toLowerCase().includes(searchTerm.toLowerCase());
    const matchesDensity = filterDensity === 'ALL' || r.trafficDensity === filterDensity;
    return matchesSearch && matchesDensity;
  });

  // Extract camera list from intersections
  const cameraFeeds = state.intersections.map((n) => ({
    nodeId: n.id,
    nodeName: n.name,
    ...n.camera,
  }));

  // Density badge helper
  const getDensityBadge = (density: string, congestion: number) => {
    switch (density) {
      case 'GREEN':
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">
            NORMAL ({congestion}%)
          </span>
        );
      case 'YELLOW':
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-amber-500/20 text-amber-400 border border-amber-500/30">
            MODERATE ({congestion}%)
          </span>
        );
      case 'ORANGE':
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-orange-500/20 text-orange-400 border border-orange-500/30">
            HEAVY ({congestion}%)
          </span>
        );
      case 'RED':
        return (
          <span className="px-2 py-0.5 rounded text-[10px] font-mono font-bold bg-rose-500/20 text-rose-400 border border-rose-500/30 animate-pulse">
            GRIDLOCK ({congestion}%)
          </span>
        );
      default:
        return null;
    }
  };

  return (
    <div className="flex flex-col space-y-6">
      {/* Top Telemetry Statistics Grid */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-4 shadow-xl">
          <div className="flex items-center justify-between">
            <span className="text-gray-400 text-xs font-mono">TOTAL MUNICIPAL ROADS</span>
            <Activity size={16} className="text-cyan-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-white font-mono">22</span>
            <span className="text-xs text-emerald-400 font-mono">100% SENSOR MONITORED</span>
          </div>
          <div className="mt-1 text-[11px] text-gray-500">Continuous 1000ms IRSA evaluation</div>
        </div>

        <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-4 shadow-xl">
          <div className="flex items-center justify-between">
            <span className="text-gray-400 text-xs font-mono">AVERAGE NETWORK SPEED</span>
            <TrendingUp size={16} className="text-emerald-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-emerald-400 font-mono">54.2</span>
            <span className="text-xs text-gray-400">km/h</span>
          </div>
          <div className="mt-1 text-[11px] text-gray-500">Freeflow speed limit: 60 km/h</div>
        </div>

        <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-4 shadow-xl">
          <div className="flex items-center justify-between">
            <span className="text-gray-400 text-xs font-mono">CAMERA PERCEPTION MESH</span>
            <Video size={16} className="text-purple-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-purple-300 font-mono">
              {cameraFeeds.filter((c) => c.health === 'ONLINE').length} / 12
            </span>
            <span className="text-xs text-purple-400 font-mono">30 FPS AI FEEDS</span>
          </div>
          <div className="mt-1 text-[11px] text-gray-500">YOLOv8 Edge Perception Array</div>
        </div>

        <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-4 shadow-xl">
          <div className="flex items-center justify-between">
            <span className="text-gray-400 text-xs font-mono">PREDICTION CONFIDENCE</span>
            <ShieldCheck size={16} className="text-cyan-400" />
          </div>
          <div className="mt-2 flex items-baseline space-x-2">
            <span className="text-2xl font-extrabold text-cyan-300 font-mono">92.4%</span>
            <span className="text-xs text-emerald-400 font-mono">HIGH FIDELITY</span>
          </div>
          <div className="mt-1 text-[11px] text-gray-500">Historical & real-time Bayesian fusion</div>
        </div>
      </div>

      {/* 1. Road Ranking & Health Evaluation Table */}
      <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-5 shadow-2xl space-y-4">
        <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between border-b border-gray-800 pb-3 gap-3">
          <div className="flex items-center space-x-2.5">
            <div className="p-2 rounded-lg bg-emerald-500/10 border border-emerald-500/20 text-emerald-400">
              <Activity size={18} />
            </div>
            <div>
              <h3 className="font-bold text-gray-100 text-sm tracking-wide">
                CITY ROAD NETWORK HEALTH & SITUATION MATRIX
              </h3>
              <p className="text-xs text-gray-400">
                Live evaluation of travel times, congestion tiers, suitability ratings and active incidents
              </p>
            </div>
          </div>

          {/* Search & Filter Bar */}
          <div className="flex items-center space-x-2 w-full sm:w-auto">
            <div className="relative flex-1 sm:w-48">
              <Search size={14} className="absolute left-2.5 top-2.5 text-gray-500" />
              <input
                type="text"
                placeholder="Search road..."
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
                className="w-full bg-gray-900 border border-gray-800 rounded-lg pl-8 pr-3 py-1.5 text-xs text-gray-200 placeholder-gray-500 focus:outline-none focus:border-cyan-500 font-mono"
              />
            </div>
            <select
              value={filterDensity}
              onChange={(e) => setFilterDensity(e.target.value as any)}
              className="bg-gray-900 border border-gray-800 rounded-lg px-2.5 py-1.5 text-xs text-gray-300 font-mono focus:outline-none focus:border-cyan-500"
            >
              <option value="ALL">All Tiers</option>
              <option value="GREEN">Normal (Green)</option>
              <option value="YELLOW">Moderate (Yellow)</option>
              <option value="ORANGE">Heavy (Orange)</option>
              <option value="RED">Gridlock (Red)</option>
            </select>
          </div>
        </div>

        {/* Scrollable Road Health Table */}
        <div className="overflow-x-auto rounded-lg border border-gray-800">
          <table className="w-full text-left text-xs font-mono">
            <thead className="bg-[#0E1526] text-gray-400 uppercase text-[10px] tracking-wider border-b border-gray-800">
              <tr>
                <th className="py-2.5 px-3">Corridor</th>
                <th className="py-2.5 px-3">Connection</th>
                <th className="py-2.5 px-3">Length</th>
                <th className="py-2.5 px-3">Speed</th>
                <th className="py-2.5 px-3">Congestion Tier</th>
                <th className="py-2.5 px-3">Travel Time (Cur / Pred)</th>
                <th className="py-2.5 px-3">Suitability</th>
                <th className="py-2.5 px-3">Condition / Incident</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-gray-800/60 bg-[#0B101D]">
              {filteredRoads.map((road) => (
                <tr key={road.id} className="hover:bg-gray-900/40 transition">
                  <td className="py-2.5 px-3 font-bold text-gray-200">
                    <div>{road.name}</div>
                    <span className="text-[10px] text-gray-500">{road.id}</span>
                  </td>
                  <td className="py-2.5 px-3 text-gray-400">
                    <span className="text-cyan-400">N{road.fromNodeId}</span> ↔{' '}
                    <span className="text-cyan-400">N{road.toNodeId}</span>
                  </td>
                  <td className="py-2.5 px-3 text-gray-300">{road.lengthKm} km</td>
                  <td className="py-2.5 px-3 text-gray-300">{road.speedLimitKmh} km/h</td>
                  <td className="py-2.5 px-3">
                    {getDensityBadge(road.trafficDensity, road.congestionPercent)}
                  </td>
                  <td className="py-2.5 px-3">
                    <span className="text-emerald-400 font-bold">{road.travelTimeSeconds}s</span>
                    <span className="text-gray-500 mx-1">/</span>
                    <span className="text-amber-400">{road.predictedNext5Min}s (+5m)</span>
                  </td>
                  <td className="py-2.5 px-3">
                    <div className="flex items-center space-x-1.5">
                      <span className="font-bold text-cyan-300">{road.suitabilityScore}</span>
                      <div className="w-12 bg-gray-800 h-1.5 rounded-full overflow-hidden">
                        <div
                          className="bg-cyan-500 h-full rounded-full"
                          style={{ width: `${road.suitabilityScore}%` }}
                        />
                      </div>
                    </div>
                  </td>
                  <td className="py-2.5 px-3">
                    {road.isBlocked ? (
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-rose-500/20 text-rose-300 border border-rose-500/30">
                        🛑 BLOCKED
                      </span>
                    ) : road.hasAccident ? (
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-rose-500/20 text-rose-300 border border-rose-500/30">
                        💥 ACCIDENT
                      </span>
                    ) : road.isFlooded ? (
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-blue-500/20 text-blue-300 border border-blue-500/30">
                        🌊 FLOODED
                      </span>
                    ) : road.isUnderConstruction ? (
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-amber-500/20 text-amber-300 border border-amber-500/30">
                        🚧 ROADWORKS
                      </span>
                    ) : road.isBridgeClosed ? (
                      <span className="px-2 py-0.5 rounded text-[10px] font-bold bg-rose-500/20 text-rose-300 border border-rose-500/30">
                        🌉 CLOSED
                      </span>
                    ) : (
                      <span className="text-gray-400 text-[11px]">{road.roadCondition}</span>
                    )}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* 2. AI Camera Perception Array */}
      <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-5 shadow-2xl space-y-4">
        <div className="flex items-center justify-between border-b border-gray-800 pb-3">
          <div className="flex items-center space-x-2.5">
            <div className="p-2 rounded-lg bg-purple-500/10 border border-purple-500/20 text-purple-400">
              <Video size={18} />
            </div>
            <div>
              <h3 className="font-bold text-gray-100 text-sm tracking-wide">
                AI VISION & CAMERA PERCEPTION FEEDS
              </h3>
              <p className="text-xs text-gray-400">
                Real-time optical vehicle counts, queue tracking and emergency vehicle vision detection
              </p>
            </div>
          </div>
          <span className="px-2.5 py-1 text-xs font-mono font-bold rounded bg-purple-500/20 text-purple-300 border border-purple-500/30">
            YOLOV8 EDGE MODEL
          </span>
        </div>

        {/* Camera Cards Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-3">
          {cameraFeeds.map((cam) => (
            <div
              key={cam.id}
              className="bg-[#0E1526] rounded-xl p-3.5 border border-gray-800 flex flex-col justify-between hover:border-gray-700 transition"
            >
              <div>
                <div className="flex items-center justify-between mb-2">
                  <span className="text-xs font-mono font-bold text-gray-200 flex items-center">
                    <Video size={13} className="mr-1.5 text-purple-400" />
                    {cam.id}
                  </span>
                  <span
                    className={`text-[9px] font-mono px-1.5 py-0.5 rounded font-bold ${
                      cam.health === 'ONLINE'
                        ? 'bg-emerald-500/20 text-emerald-400'
                        : 'bg-rose-500/20 text-rose-400'
                    }`}
                  >
                    {cam.health} ({cam.fps} FPS)
                  </span>
                </div>

                <div className="text-[11px] font-semibold text-gray-400 mb-2 truncate">
                  Node #{cam.nodeId}: {cam.nodeName}
                </div>

                <div className="space-y-1.5 bg-gray-900/60 p-2.5 rounded border border-gray-800/80 font-mono text-[11px]">
                  <div className="flex justify-between text-gray-300">
                    <span className="text-gray-400">Vehicle Count:</span>
                    <span className="font-bold text-cyan-300">{cam.vehicleCount}</span>
                  </div>
                  <div className="flex justify-between text-gray-300">
                    <span className="text-gray-400">Lane Queue:</span>
                    <span className="font-bold text-amber-300">{cam.queueLength} vehicles</span>
                  </div>
                  <div className="flex justify-between text-gray-300">
                    <span className="text-gray-400">Lane Occupancy:</span>
                    <span className="font-bold text-gray-200">{cam.laneOccupancyPercent}%</span>
                  </div>
                  <div className="flex justify-between text-gray-300 pt-1 border-t border-gray-800">
                    <span className="text-gray-400">AI Sirens Vision:</span>
                    <span
                      className={`font-bold ${
                        cam.emergencyVehicleDetected ? 'text-emerald-400' : 'text-gray-500'
                      }`}
                    >
                      {cam.emergencyVehicleDetected ? 'DETECTED' : 'CLEAR'}
                    </span>
                  </div>
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
