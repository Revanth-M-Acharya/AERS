/**
 * @file CircularPriorityMeter.tsx
 * @brief High-precision animated circular priority gauge.
 * Displays real-time IRSA 0-100 score with glowing sweep and P1-P4 classification.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import { PriorityGrade } from '../../types';

interface CircularPriorityMeterProps {
  score: number;
  priorityClass: PriorityGrade;
  priorityLabel: string;
  color: string;
  size?: number;
}

export const CircularPriorityMeter: React.FC<CircularPriorityMeterProps> = ({
  score,
  priorityClass,
  priorityLabel,
  color,
  size = 220,
}) => {
  const strokeWidth = 14;
  const radius = (size - strokeWidth) / 2;
  const circumference = 2 * Math.PI * radius;
  // Sweep arc: 270 degrees total
  const arcOffset = circumference * 0.25;
  const strokeDashoffset = circumference - (score / 100) * (circumference * 0.75);

  const getPillBg = () => {
    switch (priorityClass) {
      case 'P1_CRITICAL':
        return 'bg-red-500/20 text-red-400 border-red-500/40 shadow-[0_0_15px_rgba(239,68,68,0.4)]';
      case 'P2_HIGH':
        return 'bg-orange-500/20 text-orange-400 border-orange-500/40 shadow-[0_0_15px_rgba(249,115,22,0.4)]';
      case 'P3_MEDIUM':
        return 'bg-yellow-500/20 text-yellow-400 border-yellow-500/40 shadow-[0_0_15px_rgba(234,179,8,0.4)]';
      case 'P4_LOW':
      default:
        return 'bg-emerald-500/20 text-emerald-400 border-emerald-500/40 shadow-[0_0_15px_rgba(16,185,129,0.4)]';
    }
  };

  return (
    <div className="flex flex-col items-center justify-center relative select-none">
      <div className="relative" style={{ width: size, height: size }}>
        <svg className="w-full h-full -rotate-90 transform" viewBox={`0 0 ${size} ${size}`}>
          {/* Background Track */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke="#1E293B"
            strokeWidth={strokeWidth}
            strokeDasharray={`${circumference * 0.75} ${circumference * 0.25}`}
            strokeLinecap="round"
          />
          {/* Colored Progress Arc */}
          <circle
            cx={size / 2}
            cy={size / 2}
            r={radius}
            fill="none"
            stroke={color}
            strokeWidth={strokeWidth}
            strokeDasharray={circumference}
            strokeDashoffset={strokeDashoffset}
            strokeLinecap="round"
            className="transition-all duration-700 ease-out"
            style={{
              filter: `drop-shadow(0 0 10px ${color})`,
            }}
          />
        </svg>

        {/* Center Content */}
        <div className="absolute inset-0 flex flex-col items-center justify-center text-center">
          <span className="text-xs uppercase font-mono text-gray-400 tracking-widest mb-0.5">IRSA Score</span>
          <span className="text-5xl font-black font-mono tracking-tight" style={{ color }}>
            {score}
          </span>
          <span className="text-[11px] text-gray-400 font-mono mt-0.5">/ 100</span>

          <div className="mt-2">
            <span className={`px-3 py-1 rounded-full text-xs font-black font-mono tracking-wide border ${getPillBg()}`}>
              {priorityLabel}
            </span>
          </div>
        </div>
      </div>

      <div className="mt-3 text-center">
        <div className="text-[11px] font-mono text-gray-400">
          Target Preemption: <span className="text-white font-bold">{priorityClass === 'P1_CRITICAL' ? 'ALL-RED LOCKDOWN' : 'GREEN WAVE'}</span>
        </div>
      </div>
    </div>
  );
};
