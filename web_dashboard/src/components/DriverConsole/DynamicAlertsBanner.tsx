/**
 * @file DynamicAlertsBanner.tsx
 * @brief Flash Alert System for Ambulance Driver Console.
 * Displays high-priority tactical road closures, hospital statuses, node drops, and reroute alerts.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useState } from 'react';
import { DriverAlert } from '../../types';
import { voiceNav } from '../../services/voiceNavigationService';
import {
  AlertTriangle,
  ServerCrash,
  Hospital,
  Car,
  Shield,
  Shuffle,
  WifiOff,
  X,
  BellRing,
} from 'lucide-react';

interface DynamicAlertsBannerProps {
  alerts: DriverAlert[];
  onDismissAlert?: (id: string) => void;
  onTriggerAlert?: (type: DriverAlert['type']) => void;
}

export const DynamicAlertsBanner: React.FC<DynamicAlertsBannerProps> = ({
  alerts,
  onDismissAlert,
  onTriggerAlert,
}) => {
  const getIcon = (type: DriverAlert['type']) => {
    switch (type) {
      case 'ROAD_CLOSED':
        return <AlertTriangle className="text-red-400" size={18} />;
      case 'NODE_FAILURE':
        return <ServerCrash className="text-red-400" size={18} />;
      case 'HOSPITAL_BUSY':
        return <Hospital className="text-amber-400" size={18} />;
      case 'TRAFFIC_CONGESTION':
        return <Car className="text-amber-400" size={18} />;
      case 'POLICE_ESCORT':
        return <Shield className="text-blue-400" size={18} />;
      case 'ALTERNATIVE_ROUTE':
        return <Shuffle className="text-cyan-400" size={18} />;
      case 'COMMUNICATION_LOST':
      default:
        return <WifiOff className="text-rose-400" size={18} />;
    }
  };

  const alertTriggers: Array<{ type: DriverAlert['type']; label: string; speech: string }> = [
    { type: 'ROAD_CLOSED', label: 'Road Closed', speech: 'Critical Alert: Road Closed ahead at Intersection 2.' },
    { type: 'NODE_FAILURE', label: 'Node Failure', speech: 'Node Failure detected: Intersection 2 dropped off QNX network.' },
    { type: 'ALTERNATIVE_ROUTE', label: 'Alternative Route', speech: 'Alternative Route Found. Queens Road Bypass engaged.' },
    { type: 'HOSPITAL_BUSY', label: 'Hospital Busy', speech: 'Hospital Busy: Trauma bay diverted to Metro Heart.' },
    { type: 'TRAFFIC_CONGESTION', label: 'Traffic Congestion', speech: 'Heavy Traffic Congestion detected on primary corridor.' },
    { type: 'POLICE_ESCORT', label: 'Police Escort', speech: 'Police Escort Cruiser dispatched to lead corridor.' },
    { type: 'COMMUNICATION_LOST', label: 'Comm Lost', speech: 'Warning: Communication link degraded. Reconnecting.' },
  ];

  return (
    <div className="space-y-3 font-mono">
      {/* Active Alerts List */}
      {alerts.length > 0 && (
        <div className="space-y-2">
          {alerts.map((alert) => (
            <div
              key={alert.id}
              className={`p-3 rounded-xl border flex items-center justify-between shadow-lg animate-pulse transition-all ${
                alert.severity === 'CRITICAL'
                  ? 'bg-rose-950/80 border-rose-500 text-rose-100 shadow-[0_0_15px_rgba(244,63,94,0.3)]'
                  : alert.severity === 'WARNING'
                  ? 'bg-amber-950/80 border-amber-500 text-amber-100 shadow-[0_0_15px_rgba(245,158,11,0.3)]'
                  : 'bg-cyan-950/80 border-cyan-500 text-cyan-100'
              }`}
            >
              <div className="flex items-center space-x-3">
                <div className="p-1.5 rounded-lg bg-black/40 flex-shrink-0">
                  {getIcon(alert.type)}
                </div>
                <div>
                  <div className="text-xs font-bold uppercase tracking-wider">{alert.title}</div>
                  <div className="text-[11px] opacity-90">{alert.message}</div>
                </div>
              </div>

              {onDismissAlert && (
                <button
                  onClick={() => onDismissAlert(alert.id)}
                  className="p-1 hover:bg-black/30 rounded text-gray-300 hover:text-white transition"
                  title="Acknowledge & Dismiss"
                >
                  <X size={15} />
                </button>
              )}
            </div>
          ))}
        </div>
      )}

      {/* Manual Alert Trigger Bar (For Judge Testing) */}
      <div className="bg-[#0A0F1D] border border-slate-800 rounded-xl p-3">
        <div className="text-[10px] text-gray-400 uppercase tracking-widest flex items-center space-x-1.5 mb-2">
          <BellRing size={12} className="text-cyan-400" />
          <span>Simulate Tactical Flash Alerts:</span>
        </div>

        <div className="flex flex-wrap gap-1.5 text-xs">
          {alertTriggers.map(({ type, label, speech }) => (
            <button
              key={type}
              onClick={() => {
                if (onTriggerAlert) onTriggerAlert(type);
                voiceNav.speak(speech, type === 'ROAD_CLOSED' || type === 'NODE_FAILURE' ? 'HIGH' : 'NORMAL');
              }}
              className="px-2.5 py-1 rounded bg-slate-900 border border-slate-700 text-gray-300 hover:text-white hover:border-cyan-500 text-[11px] transition shadow-sm"
            >
              ⚡ {label}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
};
