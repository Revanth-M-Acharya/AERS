/**
 * @file SeverityBreakdownChart.tsx
 * @brief Multi-dimensional severity breakdown visualization for IRSA Engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import { IRSAResult } from '../../types';
import { Activity, Car, CloudRain, Compass, Building, Wrench } from 'lucide-react';

interface SeverityBreakdownChartProps {
  breakdown: IRSAResult['severityBreakdown'];
}

export const SeverityBreakdownChart: React.FC<SeverityBreakdownChartProps> = ({ breakdown }) => {
  const items = [
    { label: 'Medical Severity', value: breakdown.medicalSeverity, icon: Activity, color: 'bg-rose-500', text: 'text-rose-400' },
    { label: 'Traffic Density / Congestion', value: breakdown.trafficSeverity, icon: Car, color: 'bg-amber-500', text: 'text-amber-400' },
    { label: 'Environmental / Weather', value: breakdown.environmentalSeverity, icon: CloudRain, color: 'bg-cyan-500', text: 'text-cyan-400' },
    { label: 'Route Complexity', value: breakdown.routeComplexity, icon: Compass, color: 'bg-purple-500', text: 'text-purple-400' },
    { label: 'Hospital Availability', value: breakdown.hospitalAvailability, icon: Building, color: 'bg-emerald-500', text: 'text-emerald-400' },
    { label: 'Vehicle Readiness', value: breakdown.vehicleReadiness, icon: Wrench, color: 'bg-blue-500', text: 'text-blue-400' },
  ];

  return (
    <div className="space-y-3.5">
      {items.map((item) => {
        const Icon = item.icon;
        return (
          <div key={item.label} className="space-y-1">
            <div className="flex items-center justify-between text-xs font-mono">
              <div className="flex items-center space-x-1.5 text-gray-300">
                <Icon size={14} className={item.text} />
                <span>{item.label}</span>
              </div>
              <span className="font-bold text-white">{item.value}%</span>
            </div>
            <div className="h-2 w-full bg-slate-800 rounded-full overflow-hidden border border-slate-700/50">
              <div
                className={`h-full ${item.color} rounded-full transition-all duration-700 ease-out`}
                style={{ width: `${item.value}%` }}
              />
            </div>
          </div>
        );
      })}
    </div>
  );
};
