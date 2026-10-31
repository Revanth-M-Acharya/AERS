import React from 'react';
import {
  LayoutDashboard,
  MapPin,
  ShieldAlert,
  PhoneCall,
  Ambulance,
  TrafficCone,
  Building2,
  Cpu,
  BarChart3,
  RotateCcw,
  ScrollText,
  Settings,
  Activity,
  Radio,
  Network,
  SlidersHorizontal,
  Navigation,
} from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';

export type PageId =
  | 'dashboard'
  | 'priority-scheduler'
  | 'driver-console'
  | 'irsa'
  | 'city-map'
  | 'command-center'
  | 'emergency-calls'
  | 'ambulances'
  | 'traffic-signals'
  | 'hospitals'
  | 'digital-twin'
  | 'analytics'
  | 'replay'
  | 'logs'
  | 'settings';

interface SidebarProps {
  activePage: PageId;
  onSelectPage: (page: PageId) => void;
}

export const Sidebar: React.FC<SidebarProps> = ({ activePage, onSelectPage }) => {
  const { state, mode } = useSimulation();

  const navItems = [
    { id: 'dashboard' as PageId, label: 'Dashboard', icon: LayoutDashboard },
    { id: 'priority-scheduler' as PageId, label: 'Priority Scheduler', icon: SlidersHorizontal, badge: 'IRSA' },
    { id: 'driver-console' as PageId, label: 'Driver Console', icon: Navigation, badge: 'HUD' },
    { id: 'irsa' as PageId, label: 'Road Situation (IRSA)', icon: Network, badge: 'CITY' },
    { id: 'city-map' as PageId, label: 'City Map', icon: MapPin },
    { id: 'command-center' as PageId, label: 'Command Center', icon: ShieldAlert, badge: state.isRunning ? 'ACTIVE' : undefined },
    { id: 'emergency-calls' as PageId, label: 'Emergency Calls', icon: PhoneCall },
    { id: 'ambulances' as PageId, label: 'Ambulances', icon: Ambulance },
    { id: 'traffic-signals' as PageId, label: 'Traffic Signals', icon: TrafficCone },
    { id: 'hospitals' as PageId, label: 'Hospitals', icon: Building2 },
    { id: 'digital-twin' as PageId, label: 'Digital Twin', icon: Cpu, badge: 'SYNC' },
    { id: 'analytics' as PageId, label: 'Analytics', icon: BarChart3 },
    { id: 'replay' as PageId, label: 'Mission Replay', icon: RotateCcw },
    { id: 'logs' as PageId, label: 'Event Logs', icon: ScrollText },
    { id: 'settings' as PageId, label: 'Settings', icon: Settings },
  ];

  return (
    <aside className="w-64 bg-[#0D121F] border-r border-gray-800 flex flex-col h-screen select-none z-30">
      {/* Brand Header */}
      <div className="p-4 border-b border-gray-800/80 flex items-center space-x-3 bg-gradient-to-r from-blue-950/30 to-transparent">
        <div className="w-10 h-10 rounded-lg bg-cyan-500/10 border border-cyan-400/30 flex items-center justify-center text-cyan-400 shadow-[0_0_12px_rgba(0,240,255,0.3)]">
          <Activity className="w-6 h-6 animate-pulse" />
        </div>
        <div>
          <div className="flex items-center space-x-1.5">
            <span className="font-heading-cyber text-lg font-bold tracking-wider text-white">SMART AEGIS</span>
            <span className="text-[10px] px-1.5 py-0.5 rounded bg-cyan-500/20 text-cyan-300 font-mono border border-cyan-400/30">v1.0</span>
          </div>
          <p className="text-[11px] text-gray-400 font-mono tracking-tight">QNX SDP 8.0 Ops Center</p>
        </div>
      </div>

      {/* Mode Status Pill */}
      <div className="px-4 py-2.5 bg-gray-900/50 border-b border-gray-800/60 flex items-center justify-between text-xs">
        <span className="text-gray-400 font-medium">Operation Mode:</span>
        <div className="flex items-center space-x-1.5">
          <span className={`w-2 h-2 rounded-full ${mode === 'LIVE' ? 'bg-emerald-400 animate-ping' : 'bg-cyan-400'}`} />
          <span className={`font-mono font-bold text-[11px] ${mode === 'LIVE' ? 'text-emerald-400' : 'text-cyan-400'}`}>
            {mode}
          </span>
        </div>
      </div>

      {/* Navigation List */}
      <nav className="flex-1 p-2 space-y-1 overflow-y-auto">
        {navItems.map((item) => {
          const Icon = item.icon;
          const isActive = activePage === item.id;
          return (
            <button
              key={item.id}
              onClick={() => onSelectPage(item.id)}
              className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-lg text-xs font-medium transition-all ${
                isActive
                  ? 'bg-cyan-500/15 text-cyan-300 border border-cyan-500/30 shadow-[0_0_15px_rgba(0,240,255,0.15)] font-semibold'
                  : 'text-gray-400 hover:text-gray-200 hover:bg-gray-800/60'
              }`}
            >
              <div className="flex items-center space-x-3">
                <Icon className={`w-4 h-4 ${isActive ? 'text-cyan-400' : 'text-gray-500'}`} />
                <span>{item.label}</span>
              </div>
              {item.badge && (
                <span
                  className={`text-[10px] font-mono px-1.5 py-0.5 rounded ${
                    item.badge === 'ACTIVE'
                      ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40 animate-pulse'
                      : 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30'
                  }`}
                >
                  {item.badge}
                </span>
              )}
            </button>
          );
        })}
      </nav>

      {/* Cluster Footer Card */}
      <div className="p-3 border-t border-gray-800/80 bg-gray-950/60">
        <div className="flex items-center justify-between text-[11px] mb-1.5">
          <span className="text-gray-400">Cluster Coordinator:</span>
          <span className="text-emerald-400 font-mono font-semibold flex items-center space-x-1">
            <Radio className="w-3 h-3 text-emerald-400 animate-pulse" />
            <span>ONLINE</span>
          </span>
        </div>
        <div className="text-[10px] font-mono text-gray-500 flex justify-between">
          <span>Pi 1: 10.204.179.4</span>
          <span>Mesh: TCP/IP</span>
        </div>
      </div>
    </aside>
  );
};
