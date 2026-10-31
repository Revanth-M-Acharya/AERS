/**
 * @file AutomotiveCluster.tsx
 * @brief High-contrast automotive instrument cluster for Ambulance Driver Dashboard.
 * Displays speed, next turn banner, remaining distance, ETA, and telematics.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import {
  Navigation,
  ArrowUp,
  ArrowUpRight,
  ArrowUpLeft,
  Gauge,
  Wifi,
  Fuel,
  Radio,
  MapPin,
  Mic,
  Clock,
  ShieldAlert,
} from 'lucide-react';
import { AmbulanceTelemetry } from '../../types';

interface AutomotiveClusterProps {
  ambulance: AmbulanceTelemetry;
  nextTurnText: string;
  nextTurnDistanceMeters: number;
  turnDirection: 'LEFT' | 'RIGHT' | 'STRAIGHT';
  fuelPercent?: number;
  satellites?: number;
  driverName?: string;
}

export const AutomotiveCluster: React.FC<AutomotiveClusterProps> = ({
  ambulance,
  nextTurnText,
  nextTurnDistanceMeters,
  turnDirection,
  fuelPercent = 86,
  satellites = 14,
  driverName = 'Officer R. Kumar',
}) => {
  const renderTurnIcon = () => {
    switch (turnDirection) {
      case 'LEFT':
        return <ArrowUpLeft size={36} className="text-cyan-400 animate-bounce" />;
      case 'RIGHT':
        return <ArrowUpRight size={36} className="text-cyan-400 animate-bounce" />;
      case 'STRAIGHT':
      default:
        return <ArrowUp size={36} className="text-emerald-400 animate-pulse" />;
    }
  };

  return (
    <div className="bg-[#0A0F1D] border border-cyan-500/30 rounded-2xl p-5 shadow-[0_0_30px_rgba(0,240,255,0.1)] text-white space-y-4">
      {/* Top Telematics Status Bar */}
      <div className="flex items-center justify-between text-xs font-mono pb-3 border-b border-gray-800 text-gray-400">
        <div className="flex items-center space-x-4">
          <div className="flex items-center space-x-1.5 text-cyan-300">
            <span className="w-2 h-2 rounded-full bg-cyan-400 animate-ping" />
            <span className="font-bold">{ambulance.id}</span>
            <span className="text-gray-500">•</span>
            <span>{driverName}</span>
          </div>

          <div className="hidden sm:flex items-center space-x-1 text-emerald-400">
            <Wifi size={13} />
            <span>5G QNX SYNC (0.8ms)</span>
          </div>
        </div>

        <div className="flex items-center space-x-4">
          <div className="flex items-center space-x-1 text-gray-300">
            <MapPin size={13} className="text-amber-400" />
            <span>GPS: 12.9716°N, 77.5946°E ({satellites} Sats)</span>
          </div>

          <div className="flex items-center space-x-1.5">
            <Fuel size={13} className={fuelPercent < 25 ? 'text-rose-400' : 'text-emerald-400'} />
            <span className="font-bold">{fuelPercent}%</span>
          </div>

          <div className="flex items-center space-x-1 text-cyan-400">
            <Mic size={13} />
            <span className="text-[10px] bg-cyan-950/60 px-1.5 py-0.5 rounded border border-cyan-500/30">RADIO ON</span>
          </div>
        </div>
      </div>

      {/* Main Cluster Grid */}
      <div className="grid grid-cols-1 md:grid-cols-12 gap-4 items-center">
        {/* Speedometer (4 cols) */}
        <div className="md:col-span-4 bg-slate-900/90 border border-slate-800 rounded-xl p-4 flex flex-col items-center justify-center relative overflow-hidden">
          <div className="absolute top-2 left-3 text-[10px] font-mono text-gray-400 uppercase tracking-widest flex items-center space-x-1">
            <Gauge size={12} className="text-cyan-400" />
            <span>Vehicle Speed</span>
          </div>

          <div className="mt-3 flex items-baseline space-x-1">
            <span className="text-6xl font-black font-mono tracking-tighter text-white">
              {ambulance.speedKmh}
            </span>
            <span className="text-sm font-mono text-cyan-400 font-bold uppercase">km/h</span>
          </div>

          <div className="w-full bg-slate-800 h-2 rounded-full mt-3 overflow-hidden">
            <div
              className={`h-full rounded-full transition-all duration-300 ${
                ambulance.speedKmh > 70
                  ? 'bg-rose-500'
                  : ambulance.speedKmh > 40
                  ? 'bg-cyan-400'
                  : 'bg-emerald-400'
              }`}
              style={{ width: `${Math.min(100, (ambulance.speedKmh / 90) * 100)}%` }}
            />
          </div>

          <div className="text-[10px] font-mono text-gray-400 mt-2 flex justify-between w-full">
            <span>0</span>
            <span>SPEED LIMIT: 60 KM/H</span>
            <span>90</span>
          </div>
        </div>

        {/* Tactical Turn-by-Turn Guidance Banner (5 cols) */}
        <div className="md:col-span-5 bg-gradient-to-r from-blue-950/60 via-slate-900 to-slate-900 border border-cyan-500/40 rounded-xl p-4 flex items-center space-x-4 shadow-[0_0_20px_rgba(6,182,212,0.15)]">
          <div className="p-3 bg-cyan-500/10 rounded-xl border border-cyan-500/30 flex-shrink-0">
            {renderTurnIcon()}
          </div>

          <div className="space-y-1">
            <div className="text-xs font-mono uppercase tracking-wider text-cyan-300 font-bold">
              In {nextTurnDistanceMeters}m:
            </div>
            <div className="text-base font-bold text-white leading-tight">
              {nextTurnText}
            </div>
            <div className="text-xs font-mono text-gray-400 flex items-center space-x-1">
              <Navigation size={12} className="text-emerald-400" />
              <span>Current: {ambulance.currentRoad}</span>
            </div>
          </div>
        </div>

        {/* ETA & Destination Summary (3 cols) */}
        <div className="md:col-span-3 bg-slate-900/90 border border-slate-800 rounded-xl p-4 space-y-2.5 font-mono">
          <div className="flex items-center justify-between text-xs">
            <span className="text-gray-400 flex items-center space-x-1">
              <Clock size={12} className="text-amber-400" />
              <span>Est. Transit</span>
            </span>
            <span className="text-lg font-black text-amber-400">
              {ambulance.etaSeconds}s
            </span>
          </div>

          <div className="flex items-center justify-between text-xs">
            <span className="text-gray-400">Distance</span>
            <span className="font-bold text-white">
              {(1 - ambulance.positionPercent / 100) > 0 ? ((1 - ambulance.positionPercent / 100) * 3.8).toFixed(1) : '0.0'} km
            </span>
          </div>

          <div className="pt-1 border-t border-slate-800">
            <div className="text-[10px] text-gray-400 uppercase">Hospital Target:</div>
            <div className="text-xs font-bold text-rose-300 truncate">{ambulance.destination}</div>
          </div>
        </div>
      </div>
    </div>
  );
};
