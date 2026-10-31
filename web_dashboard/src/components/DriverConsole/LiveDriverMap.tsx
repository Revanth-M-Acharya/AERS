/**
 * @file LiveDriverMap.tsx
 * @brief Tactical Driver Navigation Map Component.
 * Displays high-contrast route path, green wave illuminated corridor,
 * live traffic signals, blocked road markers, and dynamic ambulance position.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import { IntersectionState, RoadData, RouteOption, AmbulanceTelemetry } from '../../types';
import { Hospital, ShieldAlert, AlertTriangle, Navigation } from 'lucide-react';

interface LiveDriverMapProps {
  intersections: IntersectionState[];
  roads: RoadData[];
  activeRoute: RouteOption;
  ambulance: AmbulanceTelemetry;
}

export const LiveDriverMap: React.FC<LiveDriverMapProps> = ({
  intersections,
  roads,
  activeRoute,
  ambulance,
}) => {
  // Normalize canvas coordinate bounds (0 - 800 x 0 - 500)
  const getNode = (id: number) => intersections.find((i) => i.id === id);

  // Calculate current ambulance interpolated position along the route
  const getAmbulanceCoordinates = () => {
    if (!activeRoute || activeRoute.nodeIds.length < 2) return { x: 100, y: 250 };
    const totalSegments = activeRoute.nodeIds.length - 1;
    const progress = Math.min(1, Math.max(0, ambulance.positionPercent / 100));
    const segmentIndex = Math.min(totalSegments - 1, Math.floor(progress * totalSegments));
    const segmentProgress = (progress * totalSegments) - segmentIndex;

    const fromNode = getNode(activeRoute.nodeIds[segmentIndex]);
    const toNode = getNode(activeRoute.nodeIds[segmentIndex + 1]);

    if (!fromNode || !toNode) return { x: 100, y: 250 };

    return {
      x: fromNode.x + (toNode.x - fromNode.x) * segmentProgress,
      y: fromNode.y + (toNode.y - fromNode.y) * segmentProgress,
    };
  };

  const ambPos = getAmbulanceCoordinates();

  return (
    <div className="bg-[#060A14] border border-cyan-500/30 rounded-2xl p-4 shadow-xl relative overflow-hidden font-mono">
      {/* Map Header */}
      <div className="flex items-center justify-between pb-2 mb-2 border-b border-slate-800 text-xs">
        <div className="flex items-center space-x-2 text-cyan-300">
          <Navigation size={14} className="text-cyan-400" />
          <span className="font-bold uppercase tracking-wider text-white">Tactical Route Guidance Map</span>
        </div>
        <div className="flex items-center space-x-3 text-[11px]">
          <span className="flex items-center space-x-1 text-emerald-400">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span>Green Wave Preempted</span>
          </span>
          <span className="text-gray-400">Route: <span className="text-white font-bold">{activeRoute?.name || 'Primary Corridor'}</span></span>
        </div>
      </div>

      {/* SVG Map Viewport */}
      <div className="relative w-full aspect-[16/9] max-h-[380px] bg-[#030712] rounded-xl overflow-hidden border border-slate-800">
        <svg viewBox="0 0 800 480" className="w-full h-full">
          {/* Background Grid */}
          <defs>
            <pattern id="tacticalGrid" width="40" height="40" patternUnits="userSpaceOnUse">
              <path d="M 40 0 L 0 0 0 40" fill="none" stroke="#0F172A" strokeWidth="1" />
            </pattern>
          </defs>
          <rect width="800" height="480" fill="url(#tacticalGrid)" />

          {/* Regular Roads */}
          {roads.map((road) => {
            const n1 = getNode(road.fromNodeId);
            const n2 = getNode(road.toNodeId);
            if (!n1 || !n2) return null;
            const isCorridor = activeRoute?.roadIds?.includes(road.id);

            return (
              <line
                key={road.id}
                x1={n1.x}
                y1={n1.y}
                x2={n2.x}
                y2={n2.y}
                stroke={
                  road.isBlocked
                    ? '#EF4444'
                    : isCorridor
                    ? '#10B981'
                    : road.trafficDensity === 'RED'
                    ? '#DC2626'
                    : road.trafficDensity === 'YELLOW'
                    ? '#F59E0B'
                    : '#1E293B'
                }
                strokeWidth={isCorridor ? 6 : road.isBlocked ? 4 : 2.5}
                strokeDasharray={road.isBlocked ? '6 4' : undefined}
                strokeLinecap="round"
                className="transition-all duration-500"
              />
            );
          })}

          {/* Illuminated Active Green Corridor Glow */}
          {activeRoute?.nodeIds && activeRoute.nodeIds.length > 1 && (
            <polyline
              points={activeRoute.nodeIds
                .map((id) => {
                  const n = getNode(id);
                  return n ? `${n.x},${n.y}` : '';
                })
                .filter(Boolean)
                .join(' ')}
              fill="none"
              stroke="#00F0FF"
              strokeWidth="3"
              strokeDasharray="8 6"
              className="animate-pulse"
              style={{ filter: 'drop-shadow(0 0 8px #00F0FF)' }}
            />
          )}

          {/* Intersections (Signals & Preemption Nodes) */}
          {intersections.map((intNode) => {
            const isOnRoute = activeRoute?.nodeIds?.includes(intNode.id);
            return (
              <g key={intNode.id} className="cursor-pointer">
                {/* Node Halo */}
                <circle
                  cx={intNode.x}
                  cy={intNode.y}
                  r={isOnRoute ? 18 : 12}
                  fill={intNode.health === 'OFFLINE' ? '#7F1D1D' : intNode.blueLed ? '#0369A1' : '#0F172A'}
                  stroke={intNode.health === 'OFFLINE' ? '#EF4444' : isOnRoute ? '#00F0FF' : '#334155'}
                  strokeWidth={isOnRoute ? 2.5 : 1}
                />

                {/* Preemption Strobe Indicator */}
                {intNode.blueLed && (
                  <circle
                    cx={intNode.x}
                    cy={intNode.y}
                    r={24}
                    fill="none"
                    stroke="#00F0FF"
                    strokeWidth="1.5"
                    className="animate-ping"
                  />
                )}

                {/* Traffic Signal Color Core */}
                <circle
                  cx={intNode.x}
                  cy={intNode.y}
                  r={6}
                  fill={
                    intNode.health === 'OFFLINE'
                      ? '#EF4444'
                      : intNode.signalColor === 'GREEN'
                      ? '#10B981'
                      : intNode.signalColor === 'YELLOW'
                      ? '#F59E0B'
                      : '#EF4444'
                  }
                  style={{
                    filter: `drop-shadow(0 0 6px ${
                      intNode.signalColor === 'GREEN' ? '#10B981' : '#EF4444'
                    })`,
                  }}
                />

                {/* Node Label */}
                <text
                  x={intNode.x}
                  y={intNode.y + 28}
                  fill="#94A3B8"
                  fontSize="10"
                  fontFamily="monospace"
                  textAnchor="middle"
                  fontWeight="bold"
                >
                  {intNode.name.split(' ')[0]} #{intNode.id}
                </text>
              </g>
            );
          })}

          {/* Hospital Destination Pin */}
          <g transform="translate(680, 240)">
            <circle cx="0" cy="0" r="22" fill="#881337" stroke="#F43F5E" strokeWidth="2" />
            <text x="0" y="5" fill="#FFF" fontSize="12" textAnchor="middle" fontWeight="bold">
              H
            </text>
            <text x="0" y="32" fill="#F43F5E" fontSize="9" fontFamily="monospace" textAnchor="middle" fontWeight="bold">
              HOSPITAL BAY
            </text>
          </g>

          {/* Live Animated Ambulance Beacon */}
          <g transform={`translate(${ambPos.x}, ${ambPos.y})`}>
            {/* Beacon Ripple */}
            <circle cx="0" cy="0" r="24" fill="none" stroke="#00F0FF" strokeWidth="2" className="animate-ping" />
            {/* Outer Vehicle Circle */}
            <circle cx="0" cy="0" r="14" fill="#00F0FF" stroke="#FFFFFF" strokeWidth="2" />
            {/* Cross Symbol */}
            <path
              d="M -5 0 L 5 0 M 0 -5 L 0 5"
              stroke="#0A0F1D"
              strokeWidth="2.5"
              strokeLinecap="round"
            />
            {/* Floating Label */}
            <rect x="-30" y="-28" width="60" height="15" rx="3" fill="#0A0F1D" stroke="#00F0FF" strokeWidth="1" />
            <text x="0" y="-17" fill="#00F0FF" fontSize="9" fontFamily="monospace" textAnchor="middle" fontWeight="bold">
              AMB-101
            </text>
          </g>
        </svg>

        {/* Tactical Legend Overlays */}
        <div className="absolute bottom-2 left-2 flex items-center space-x-3 text-[10px] font-mono bg-black/75 px-3 py-1.5 rounded-lg border border-slate-800 backdrop-blur">
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-cyan-400" />
            <span className="text-gray-300">Ambulance</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-emerald-400" />
            <span className="text-gray-300">Green Signal</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-rose-500" />
            <span className="text-gray-300">Hold Red</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-blue-500 animate-ping" />
            <span className="text-cyan-300">Preemption Active</span>
          </div>
        </div>
      </div>
    </div>
  );
};
